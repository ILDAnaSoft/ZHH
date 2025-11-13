import yaml
import os.path as osp
from collections.abc import Callable
from copy import deepcopy
from ..analysis.Cuts import Cut, EqualCut, GreaterThanEqualCut, LessThanEqualCut, WithinBoundsCut, ValueCut
from ..analysis.AnalysisChannel import AnalysisChannel
from .replace_properties import replace_properties
from .replace_references import replace_references

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
    First a dict<name, AnalysisChannel> and second, a dict holding 
    <name, [cat_register_fn, cat_default, cat_order]>.
    
    Args:
        steer (_type_): _description_

    Raises:
        Exception: _description_

    Returns:
        _type_: _description_
    """

    source_map:dict[str, AnalysisChannel] = {}
    final_state_configs:dict[str, tuple[Callable, int|None, list|None]] = {}
    reset_sources:list[str] = []

    n_sources = len(steer['sources'])
    
    def per_source(name):
        path = osp.expandvars(source_spec['path'])
        fname = source_spec.get('file', 'Merged.root')

        print(f'  Reading files from <{path}>')
        source = AnalysisChannel(path, name, fname=fname)
        source.combine()
        print(f'  Found {len(source)} events')

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

        def cat_register_fn(ac:AnalysisChannel):
            for item in items:
                print(item)
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

def initialize_sources(sources:list[AnalysisChannel], final_state_configs,
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