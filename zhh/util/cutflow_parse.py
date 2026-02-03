import yaml, os, os.path as osp
import h5py
import numpy as np
from tqdm.auto import tqdm
from collections.abc import Callable
from copy import deepcopy
from glob import glob
from ..analysis.PreselectionAnalysis import set_polarization
from ..analysis.Cuts import EqualCut, GreaterThanEqualCut, LessThanEqualCut, WithinBoundsCut, ValueCut
from ..analysis.DataSource import DataSource
from .replace_properties import replace_properties
from .replace_references import replace_references
from ..data.ROOT2HDF5Converter import ROOT2HDF5Converter
from ..task.ConcurrentFuturesRunner import ProcessRunner
from typing import TypedDict, NotRequired, TYPE_CHECKING
from multiprocessing import cpu_count
from copy import deepcopy
from math import ceil

def parse_steering_file(loc:str):
    """Loads a YAML steering file, replaces references within them
    and replaces references with the EventCategories and
    FinalStateDefinitions objects. 

    Args:
        loc (str): _description_

    Returns:
        _type_: _description_
    """

    import zhh.processes.EventCategories as EventCategories
    import zhh.analysis.FinalStateDefinitions as FinalStateDefinitions

    with open(osp.expandvars(loc)) as f:
        parsed = yaml.safe_load(f)

    return replace_properties(replace_references(parsed), {
        'EventCategories': EventCategories,
        'FinalStateDefinitions': FinalStateDefinitions
    })

def process_steering(steer:dict):
    """Processes a parsed steering dictionary. Returns two dictionaries:
    First a dict<name, DataSource> and second, a dict holding 
    <name, [cat_register_fn, cat_default, cat_order]>.
    
    Args:
        steer (_type_): _description_

    Raises:
        Exception: _description_

    Returns:
        _type_: _description_
    """

    source_map:dict[str, DataSource] = {}
    final_state_configs:dict[str, tuple[Callable, int|None, list|None]] = {}
    reset_sources:list[str] = []

    n_sources = len(steer['sources'])
    
    # process beam polarization; adjust if not ILC-like
    beam_pol = steer['beam_polarization']
    assert(-1 <= beam_pol[0] <= 1)
    assert(-1 <= beam_pol[1] <= 1)

    if beam_pol[0] != -0.8 or beam_pol[1] != 0.3:
        set_polarization(beam_pol)
    
    def per_source(name):
        path = osp.expandvars(source_spec['path'])
        fname = source_spec.get('file', f'{steer["hypothesis"]}.h5')
        fpath = osp.join(path, fname)
        
        if 'interpret' in steer['features']:
            print(f'  Reading file from <{fpath}>')
            provision_feature_interpretations(steer['features']['interpret'],
                                              source_spec['root_files'], path, fname)
            
        source = DataSource(fpath, name, work_root=path)

        # make sure beam polarization fits
        with h5py.File(fpath, 'a') as hf:
            if 'beam_polarization' in hf.attrs:
                beam_pol_read = hf.attrs.get('beam_polarization', None)
                if beam_pol_read is not None:
                    assert(len(beam_pol_read) == 2)
                    assert(-1 <= beam_pol_read[0] <= 1)
                    assert(-1 <= beam_pol_read[1] <= 1)

                    if beam_pol[0] != beam_pol_read[0] or beam_pol[1] != beam_pol_read[1]:
                        # calculating the NPZ file will force source.initialize() to be called -> correct reweighting will be done automatically
                        confirm = input(f'The requested beam pol {beam_pol} does not match the saved {beam_pol_read}: Do you want to automatically reweight the events? y/n (n)').lower()

                        assert(confirm == 'y')
                        if osp.isfile(source._npz_file):
                            os.remove(source._npz_file)
            else:
                hf.attrs['beam_polarization'] = beam_pol

        # Register event category definitions
        event_categorization = source_spec['event_categorization']
        cat_default:int|None = event_categorization['default']
        cat_order:list|None = event_categorization['order']
        cat_items:list[str] = event_categorization['items']

        items:list[tuple[str, Callable, int|None]] = []

        for item in cat_items:
            found = False
            for entry in steer['event_categories']:
                if entry['name'] == item:
                    found = True
                    items.append((item, entry['handler'], entry['id']))
                    break

            if not found:
                raise Exception(f'Could not resolve event category <{item}>')

        def cat_register_fn(ac:DataSource):
            for item in items:
                ac.registerEventCategory(*item)
        
        source_map[name] = source
        final_state_configs[name] = (cat_register_fn, cat_default, cat_order)

    for i, source_spec in enumerate(steer['sources']):
        disabled = source_spec.get('disabled', False) == True
        name = source_spec['name']

        print(f'Processing source {i+1}/{n_sources} <{name}>')

        if not disabled:
            per_source(name)
            
            if source_spec.get('reset', False) == True or steer.get('reset', False) == True:
                reset_sources.append(name)
        else:
            print(f'Skipped source <{name}> (disabled)')

    return source_map, final_state_configs, reset_sources

class Interpretation(TypedDict):
    name: NotRequired[str]
    tree: NotRequired[str]
    branch: NotRequired[str]
    dtype: NotRequired[str]
    auto_increment: NotRequired[bool]
    columns: NotRequired[int]
    names: NotRequired[str]

def provision_feature_interpretations(interpretations:list[Interpretation],
                                      root_files_glob:str, path:str, file:str,
                                      integrity_check:bool=True,
                                      base_tree:str='FinalStates'):
    """_summary_

    Args:
        interpretations (list[Interpretation]): _description_
        root_files_glob (str): _description_
        path (str): _description_
        file (str): _description_
        integrity_check (bool, optional): _description_. Defaults to True.
        base_tree (str, optional): _description_. Defaults to 'FinalStates'.

    Raises:
        Exception: _description_
        Exception: _description_

    Returns:
        _type_: _description_
    """

    from zhh import tree_n_rows, AbstractTask

    ds_path = osp.expandvars(f'{path}/{file}')
    ds_meta_file = f'{osp.splitext(ds_path)[0]}.files.txt'

    root_files = sorted(glob(osp.expandvars(root_files_glob)))
    if not len(root_files):
        raise Exception(f'No ROOT files found for search mask <{root_files_glob}>')

    nrows = 0
    ds_keys = []

    # make sure files exist and nrows is set
    # read from existing files (if exist)
    if not osp.isfile(ds_path) or not osp.isfile(ds_meta_file):
        os.makedirs(osp.dirname(ds_path), exist_ok=True)
        
    with h5py.File(ds_path, 'a') as hf:
        if 'nrows' in hf.attrs:
            nrows = int(hf.attrs.get('nrows', 0))
            if nrows == 0:
                raise Exception(f'File <{ds_path}> is corrupted as it does not contain any size information.'+
                                ' Please delete the file and run provision_feature_interpretations again')
        
        ds_keys = list(hf.keys())

    if nrows == 0:
        nrows = tree_n_rows(root_files, base_tree, use_uproot=True)
        with h5py.File(ds_path, 'a') as hf:
            hf.attrs['nrows'] = nrows
    
    if integrity_check and osp.isfile(ds_meta_file):
        with open(ds_meta_file, 'rt') as tf:
            expected = tf.read()
            
        found = '\n'.join(root_files)

        if expected != found:
            expected_list = expected.split('\n')

            print('Samples expected, but not found: ', set(expected_list) - set(found))
            print('Samples found, but not expected: ', set(found) - set(expected_list))

            raise Exception('List of input files is different! Please regenerate the cache')
    
    # create HDF5 entries inside here
    bname = f'{path}/items'
    os.makedirs(osp.dirname(bname), exist_ok=True)
    #print('bname=', bname)

    to_sync = []

    def per_interpretation(feature:Interpretation, feature_names:list[str])->tuple[tuple|None, list[str]]:
        name = feature.get('name', '')
        assert(name != '')
        
        if name in feature_names:
            raise Exception(f'Invalid feature definition: Feature {name} appears at least twice')

        feature_names.append(name)

        if 'auto_increment' in feature and feature['auto_increment']:
            if not name in ds_keys:
                with h5py.File(ds_path, 'a') as hf:
                    hf[name] = np.arange(nrows, dtype=feature['dtype'] if 'dtype' in feature else 'uint32')
            
        elif 'tree' in feature:
            tree = feature['tree']
            branch = feature['branch'] if 'branch' in feature else name
            columns = feature['columns'] if 'columns' in feature else None

            if columns is not None:
                for col in range(columns):
                    if f'{name}.dim{col}' not in ds_keys:
                        return ((name, tree, branch, feature['dtype'] if 'dtype' in feature else None), feature_names)
            elif name not in ds_keys and f'{name}.dim0' not in ds_keys:
                return ((name, tree, branch, feature['dtype'] if 'dtype' in feature else None), feature_names)
        else:
            print(feature)
            raise Exception('Cannot interpret entry')
        
        return (None, feature_names)

    found = []
    feature_names:list[str] = []

    for feature in interpretations:
        if 'names' not in feature:
            assert('name' in feature)
            
            res, feature_names = per_interpretation(feature, feature_names)
            if res is not None:
                to_sync.append(res)
            else:
                found.append(feature['name'])
        else:
            for name in feature['names']:
                entry = deepcopy(feature)
                del entry['names']
                entry['name'] = name

                res, feature_names = per_interpretation(entry, feature_names)
                if res is not None:
                    to_sync.append(res)
                else:
                    found.append(name)
    
    conv_tasks:list[AbstractTask] = []
    vds_tasks:list[AbstractTask] = []
    conv_items = []
    conv_done = []
    names = []

    for item in to_sync:
        name, tree, branch, dtype = item
        names.append(name)

        conv = ROOT2HDF5Converter(root_files, ds_path, tree, branch,
                                  osp.expandvars(f'{bname}/{tree}.{branch}/item'), name, dtype)
        vds_task, item_conv_tasks = conv.convertLazy(nrows=nrows, check_existing=True)
        
        [conv_tasks.append(task) for task in item_conv_tasks]
        vds_tasks.append(vds_task)
        conv_items.append(f'{tree}.{branch} -> {name} ({dtype})')

        if len(item_conv_tasks) == 0:
            conv_done.append(name)
        #conv.convert(nrows=nrows, check_existing=True)

    if len(conv_items):
        print('Scheduling conversion of items:')
        for item in conv_items:
            print(item)

        runner = ProcessRunner(cores=ceil(cpu_count() * .8))
        runner.queueTasks(conv_tasks)
        runner.run()

        # after successful conversion, create the VDS'es
        print('Creating virtual datasets (VDSes)')

        for i, task in enumerate(pbar := tqdm(vds_tasks)):
            name = names[i]
            pbar.set_description(conv_items[i])
            deps = task.getDependencies()['conversion']
            task.run(conversion=[] if name in conv_done else [dep.getResult() for dep in deps])

    # meta file exists = everything successful
    if not osp.isfile(ds_meta_file):
        with open(ds_meta_file, 'wt') as tf:
            tf.write('\n'.join(root_files))

def initialize_sources(sources:list[DataSource], final_state_configs,
                       lumi_inv_ab:float, reset_sources:list[str]=[]):
    
    n_sources = len(sources)

    for i, source in enumerate(sources):
        src_name = source.getName()
        print(f'Initializing source {i+1}/{n_sources} <{src_name}>')

        cat_fn, cat_default, cat_order = final_state_configs[src_name]

        cat_fn(source)
        source.initialize(lumi_inv_ab, cat_default, cat_order, reset=src_name in reset_sources)

def parse_cut(x:dict)->ValueCut:
    operator:str = x['operator'].lower()

    y = deepcopy(x)
    del y['operator']
    
    match operator:
        case 'eq':
            return EqualCut(**y)
        
        case 'gte':
            if not 'lower' in y and 'value' in y:
                y['lower'] = y['value']
                del y['value']
            
            return GreaterThanEqualCut(**y)
        
        case 'lte':
            if not 'upper' in y and 'value' in y:
                y['upper'] = y['value']
                del y['value']
            
            return LessThanEqualCut(**y)
        
        case 'within_bounds':
            return WithinBoundsCut(**y)
        
        case _:
            raise Exception(f'Unknown operator {operator}')

def parse_cuts(x:list[dict])->list[ValueCut]:
    result = []

    for item in x:
        if 'disabled' in item:
            if not item['disabled']:
                del item['disabled']
            else:
                continue

        result.append(parse_cut(item))
    
    return result

def parse_steer_cutflow_table(steer:dict, **kwargs):
    cutflow_table_items:list[tuple[str, str]] = []
    cutflow_table_is_signal:list[str] = []

    for item in steer['cutflow-table']['items']:
        cutflow_table_items.append((item['label'], item['category']))

        if item.get('is_signal', False):
            cutflow_table_is_signal.append(item['category'])
    
    return (cutflow_table_items, { 'signal_categories': cutflow_table_is_signal, **kwargs })