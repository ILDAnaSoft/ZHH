from glob import glob
from typing import Optional, Union

default_locations = [
    '2f_Z_bhabhaNg',
    '2f_Z_hadronic',
    '2f_Z_nuNg',
    '2f_Z_bhabhag',
    '2f_Z_leptonic',
    '4f_ZZWWMix_hadronic',
    '4f_singleW_leptonic',
    '4f_singleZee_semileptonic',
    '4f_WW_hadronic',
    '4f_singleZee_leptonic',
    '4f_ZZ_semileptonic',
    '4f_singleW_semileptonic',
    '4f_singleZsingleWMix_leptonic',
    '4f_ZZWWMix_leptonic',
    '4f_WW_semileptonic',
    '4f_ZZ_hadronic',
    '4f_singleZnunu_leptonic',
    '4f_singleZnunu_semileptonic',
    '4f_lowmee_singleZee_leptonic',
    '4f_ZZ_leptonic',
    '4f_lowmee_singleZsingleWMix_leptonic',
    '4f_WW_leptonic',
    '5f',
    '6f_ttbar',
    '6f_yyyyZ',
    '6f_vvWW',
    '6f_eeWW',
    '6f_xxWW',
    '6f_xxxxZ',
    '6f_llWW',
    'hh']


def get_raw_files(locations:Optional[Union[str,list[str]]]=None,
                  debug:bool=False,
                  groups:Optional[list[str]]=['eL.pL', 'eL.pR', 'eR.pL', 'eR.pR']) -> list[str]:
    """Gets the full paths to mc-opt-3 sample files

    Args:
        locations (Optional[Union[str,list[str]]], optional): sub-folders to look in. if None, default_locations(). Defaults to None.
        debug (bool, optional): if True, only one file per process (and filter, if given) will be output. Defaults to False.
        groups (Optional[list[str]], optional): logically disjoint filter sets. Defaults to ['eL.pL', 'eL.pR', 'eR.pL', 'eR.pR'].

    Returns:
        list[str]: _description_
    """
    
    if locations is None:
        locations = default_locations
    elif isinstance(locations, str):
        locations = [locations]
        
    # Handle hh sample (including hh and qqh + others) separately
    if 'hh' in locations:
        locations.remove('hh')
        for loc in ['Pe1e1hh', 'Pe2e2hh', 'Pe3e3hh',
                    'Pe1e1qqh', 'Pe2e2qqh', 'Pe3e3qqh',
                    'Pn1n1hh', 'Pn23n23hh',
                    'Pn1n1qqh', 'Pn23n23qqh',
                    'Pqqhh',
                    'Pqqqqh']:
            locations.append(f'hh:{loc}')
    
    arr = []
    for location in locations:
        root_location = '/pnfs/desy.de/ilc/prod/ilc/ild/copy'
        
        if location.startswith('hh:'):
            root_location = '/pnfs/desy.de/ilc/prod/ilc/mc-2020/ild'
            carr = glob(f"{root_location}/dst-merged/500-TDR_ws/hh/ILD_l5_o1_v02/**/*.{location.replace('hh:', '')}.*.slcio", recursive=True)
        else:
            carr = glob(f"{root_location}/dst-merged/500-TDR_ws/{location}/ILD_l5_o1_v02/**/*.slcio", recursive=True)
            
        carr.sort()
        
        if groups is not None:
            carr_temp = []
            for c in carr:
                if any(map(c.__contains__, groups)):
                    carr_temp.append(c)
                    
            carr = carr_temp
                
        if debug:
            if groups is None:
                # Use any first element if no groups given
                if len(carr) > 0:
                    arr += [carr[0]]
            else:
                # Find one (the first) element per filter in the other case
                for f in groups:
                    for c in carr:
                        if f in c:
                            arr += [c]
                            break
                
        else:
            arr += carr
        
    arr.sort()
    
    return arr

#if __name__ == '__main__':
#    f = get_raw_files(debug=True)
#    print(len(f))
#    for a in f:
#        print(a)
