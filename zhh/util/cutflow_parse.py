import yaml, json, os, os.path as osp, logging, sys
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
from ..analysis import CutflowProcessor, CutflowProcessorAction, CreateCutflowPlotsAction
from typing import TypedDict, NotRequired, TYPE_CHECKING
from multiprocessing import cpu_count
from copy import deepcopy
from math import ceil

def cutflow_parse_steering_file(loc:str):
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

def cutflow_process_steering(steer:dict, integrity_check:bool=True):
    """Processes a parsed steering dictionary and handles feature interpretations.
    Returns two dictionaries: First a dict<name, DataSource> and second, a dict
    holding <name, [cat_register_fn, cat_default, cat_order]>.
    
    Args:
        steer (_type_): _description_
        integrity_check (bool): if True and an existing HDF5 file exists, will
                                check that the list of used ROOT files matches 

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
            cutflow_provision_features(steer['features']['interpret'],
                                              source_spec['root_files'], path, fname,
                                              integrity_check=integrity_check)
            
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
    clamp_min: NotRequired[float|int]
    clamp_max: NotRequired[float|int]
    nan_to: NotRequired[float|int]

def cutflow_provision_features(interpretations:list[Interpretation],
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
        base_tree (str, optional): TTree to use for estimating length. Defaults to 'FinalStates'.

    Raises:
        Exception: _description_
        Exception: _description_

    Returns:
        _type_: _description_
    """

    from zhh import tree_n_rows, AbstractTask

    ds_path = osp.expandvars(f'{path}/{file}')
    ds_meta_file = f'{osp.splitext(ds_path)[0]}.meta.json'

    root_files = sorted(glob(osp.expandvars(root_files_glob)))
    if not len(root_files):
        raise Exception(f'No ROOT files found for search mask <{root_files_glob}>')

    nrows = 0
    nrows_found = []
    ds_keys = []

    # make sure files exist and nrows is set
    # read from existing files (if exist)
    if not osp.isfile(ds_path) or not osp.isfile(ds_meta_file):
        os.makedirs(osp.dirname(ds_path), exist_ok=True)

    with h5py.File(ds_path, 'a') as hf:
        nrows = hf.attrs.get('nrows', 0)
    
    if not osp.isfile(ds_meta_file) or nrows == 0:
        nrows_found = tree_n_rows(root_files, base_tree, use_uproot=True, aggregate=False)
        
        with h5py.File(ds_path, 'a') as hf:
            nrows = hf.attrs['nrows'] = np.sum(nrows_found)

    with h5py.File(ds_path, 'a') as hf:
        if 'nrows' in hf.attrs:
            nrows = int(hf.attrs.get('nrows', 0))
            if nrows == 0:
                raise Exception(f'File <{ds_path}> is corrupted as it does not contain any size information.'+
                                ' Please delete the file and run cutflow_provision_features again')
        
        ds_keys = list(hf.keys())
    
    # for all VDS, remove any dangling references in HDF5 file at ds_path
    # they are automatically regerenated below
    with h5py.File(ds_path, 'a') as hf:
        vds_keys = []
        
        for key in ds_keys:            
            try:
                hf[key].virtual_sources()
                vds_keys.append(key)
            except RuntimeError as e:
                pass
            
        for key in vds_keys:
            for source in hf[key].virtual_sources():            
                if not os.path.exists(source.file_name):
                    print(f'Property <{key}> will be removed (dangling reference)')
                    ds_keys.remove(key)
                    del hf[key]
                    break
            
        # create mapping for quick navigation between ID entries and 
        
    
    if integrity_check and osp.isfile(ds_meta_file):
        with open(ds_meta_file, 'rt') as jf:
            meta_content = json.load(jf)
            expected = meta_content['files']
            sizes = meta_content['sizes']

        if len(expected) != len(root_files) or not all([expected[i] == root_files[i] for i in range(len(expected))]):
            print('Samples expected, but not found: ', set(expected) - set(root_files))
            print('Samples found, but not expected: ', set(root_files) - set(expected))

            raise Exception('List of input files is different! Please regenerate the cache')
        
        if nrows != np.sum(sizes):
            print(f'Encountered {nrows} rows in ROOT files, but {np.sum(sizes)} in HDF5 files. Please regenerate the cache.')
    
    # create HDF5 entries inside here
    bname = f'{path}/items'
    os.makedirs(osp.dirname(bname), exist_ok=True)
    #print('bname=', bname)

    to_sync = []

    def per_interpretation(feature:Interpretation, feature_names:list[str])->tuple[tuple|None, list[str]]:
        assert('name' in feature)
        name = feature['name']
        
        if name in feature_names:
            raise Exception(f'Invalid feature definition: Feature {name} appears at least twice')

        feature_names.append(name)

        if 'auto_increment' in feature and feature['auto_increment']:
            if not name in ds_keys:
                with h5py.File(ds_path, 'a') as hf:
                    hf[name] = np.arange(nrows, dtype=feature['dtype'] if 'dtype' in feature else 'uint32')
            
        elif 'tree' in feature:
            tree = feature['tree']
            branch = feature.get('branch', name)
            columns:int|list[str]|None = feature.get('columns', None)
            name = name.replace('/', '.')

            if columns is not None:
                if isinstance(columns, int):
                    for col in range(columns):
                        if f'{name}.dim{col}' not in ds_keys:
                            return ((name, tree, branch, feature.get('dtype', None), feature.get('nan_to', None),
                                     feature.get('clamp_min', None), feature.get('clamp_max', None)), feature_names)
                elif isinstance(columns, list) and isinstance(columns[0], str):
                    for col in columns:
                        if f'{name}.{col}' not in ds_keys:
                            #print(f'{name}.{col}', f'{name}.{col}' not in ds_keys)
                            return ((name, tree, branch, feature.get('dtype', None), feature.get('nan_to', None),
                                     feature.get('clamp_min', None), feature.get('clamp_max', None)), feature_names)
                else:
                    raise Exception(f'Property {name} not defined')
                
            elif name not in ds_keys and f'{name}.dim0' not in ds_keys:
                return ((name, tree, branch, feature.get('dtype', None), feature.get('nan_to', None),
                         feature.get('clamp_min', None), feature.get('clamp_max', None)), feature_names)
        else:
            print(feature)
            raise Exception('Cannot interpret entry')
        
        return (None, feature_names)

    found = []
    feature_names:list[str] = []
    
    #print(interpretations)

    for feature in interpretations:
        if 'names' not in feature:
            assert('name' in feature)
            
            res, feature_names = per_interpretation(feature, feature_names)
            if res is not None:
                to_sync.append(res)
            else:
                found.append(feature['name'])
        else:
            if not len(feature['names']):
                raise Exception('When using the names property, at least one item must be given')
            
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
        name, tree, branch, dtype, nan_to, clamp_min, clamp_max = item
        names.append(name)

        conv = ROOT2HDF5Converter(root_files, ds_path, tree, branch,
                                  osp.expandvars(f'{bname}/{tree}.{branch.replace("/", ".")}/item'), name, dtype, clamp=(clamp_min, clamp_max),
                                  nan_to=nan_to)
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
        with open(ds_meta_file, 'wt', encoding='utf-8') as jf:
            json.dump({
                'files': root_files,
                'sizes': nrows_found
            }, jf, ensure_ascii=False, indent=4)

def cutflow_initialize_sources(sources:list[DataSource], final_state_configs,
                       lumi_inv_ab:float, reset_sources:list[str]=[]):
    
    n_sources = len(sources)

    for i, source in enumerate(sources):
        src_name = source.getName()
        print(f'Initializing source {i+1}/{n_sources} <{src_name}>')

        cat_fn, cat_default, cat_order = final_state_configs[src_name]

        cat_fn(source)
        source.initialize(lumi_inv_ab, cat_default, cat_order, reset=src_name in reset_sources)

def cutflow_parse_cut(x:dict)->ValueCut:
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

def cutflow_parse_cuts(x:list[dict], mvas:dict={}, to_resolve:list[str]=['lower', 'upper', 'value'])->list[ValueCut]:
    
    """Parses a list of steering file cut entries. Will resolve the keys to_resolve
    using the dict mvas. e.g. in a dict { 'value': 'default_mva.threshold' , ...}
    it will look for an entry default_mva in mvas and then get the property threshold.
    If a key should be resolved, different parts should be separated by dots.

    Args:
        x (list[dict]): _description_
        mvas (dict, optional): _description_. Defaults to {}.

    Returns:
        list[ValueCut]: _description_
    """
    result = []

    from zhh import find_property

    for item in x:
        item = deepcopy(item)

        if 'disabled' in item:
            if not item['disabled']:
                del item['disabled']
            else:
                continue
        
        for prop in to_resolve:
            #print(prop, item)
            if prop in item:
                if isinstance(item[prop], str):
                    props = item[prop].split('.')
                                        
                    if not props[0] in mvas:
                        raise KeyError(f'Could not find MVA with name <{props[0]}> in registered MVAs ({mvas.keys()}) provided to cutflow_parse_cuts')
                
                    value = mvas[props[0]]
                    props.pop(0)

                    item[prop] = find_property(value, props)
                    #print(item[prop], prop)

        result.append(cutflow_parse_cut(item))
    
    return result

def cutflow_parse_steer_cutflow_table(steer:dict, **kwargs):
    cutflow_table_items:list[tuple[str, str]] = []
    cutflow_table_is_signal:list[str] = []

    for item in steer['cutflow_table']['items']:
        cutflow_table_items.append((item['label'], item['category']))

        if item.get('is_signal', False):
            cutflow_table_is_signal.append(item['category'])
    
    return (cutflow_table_items, { 'signal_categories': cutflow_table_is_signal, **kwargs })

def cutflow_register_mvas(steer:dict, cp:CutflowProcessor):
    if 'mvas' in steer:
        for mva_spec in steer['mvas']:            
            if not mva_spec['name'] in cp._mvas:
                cp.registerMVA(**mva_spec)

def cutflow_parse_actions(steer:dict, cp:CutflowProcessor):
    action_map = {}

    def per_subclass(cls):
        for subcls in cls.__subclasses__():
            if hasattr(subcls, '__subclasses__') and len(subcls.__subclasses__()):
                per_subclass(subcls)
            else:
                action_map[subcls.__name__] = subcls

    per_subclass(CutflowProcessorAction)
    
    actions = []

    for action in steer['actions']:
        if 'disabled' in action and action['disabled']:
            print(f'Skipping action <{action["type"]}>')
            continue
        
        try:
            kwargs = deepcopy(action)
            del kwargs['type']
            kwargs['cp'] = cp
            kwargs['steer'] = steer
            
            actions.append(action_map[action['type'] + 'Action'](**kwargs))
        except Exception as e:
            print(e)
            raise Exception(f'Could not instantiate action of type <{action["type"]}>')
    
    return actions

def cutflow_execute_actions(actions:list[CutflowProcessorAction], check_only:bool=False,
                            force_rerun:bool=False, log_level=logging.INFO):
    todo:list[CutflowProcessorAction] = []
    todo_idx:list[int] = []

    # first data transforming action to run
    first_to_run = -1

    for i, action in enumerate(actions):
        # clean outputs (if force_rerun)
        if action.complete() and action.transforms_data and force_rerun:
            actions[i].reset()
            
        elif not action.complete() or (action.transforms_data and -1 < first_to_run < i):
            if action.transforms_data and first_to_run == -1:
                first_to_run = i

            todo_idx.append(i)                
    
    logger = logging.getLogger('CutflowProcessorAction')
    #logger.addHandler(logging.StreamHandler(stream=sys.stdout))
    logger.setLevel(log_level)

    for i in todo_idx:
        logger.info(f'+ Scheduling action <{actions[i].__class__.__name__}>')

        todo.append(actions[i])

    if force_rerun or not check_only:
        for i, action in enumerate(todo):
            logger.info(f'. Running action {i+1}/{len(todo)} <{actions[i].__class__.__name__}>')
            action.run()

            if action.complete():
                logger.info(f'+ Finished action {i+1}/{len(todo)} <{actions[i].__class__.__name__}>')
            else:
                logger.error(f'! Could not finalize action {i+1}/{len(todo)} <{actions[i].__class__.__name__}>')

    return todo