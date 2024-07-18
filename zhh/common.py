from glob import glob
from typing import Optional, Union

pdg_map = {
    1: 'd',
    2: 'u',
    3: 's',
    4: 'c',
    5: 'b',
    6: 't',
    11: 'e',
    12: 'nu_e',
    13: 'mu',
    14: 'nu_mu',
    15: 'tau',
    16: 'nu_tau',
    21: 'g',
    22: 'gamma',
    23: 'Z',
    24: 'W',
    25: 'H',
    211: 'pi',
    111: 'pi0',
    130: 'K_L0',
    310: 'K_S0',
    321: 'K',
    2112: 'n',
    2212: 'p',
    3122: 'Lambda',    
}

pdg_groups = {
    'l': [ 11, 12, 13, 14, 15, 16 ],
    'q': [ 1, 2, 3, 4, 5, 6 ]
}

default_processes = [
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
    '6f_llWW']


def get_raw_files(processes:Optional[Union[str,list[str]]]=None,
                  debug:bool=False,
                  groups:Optional[list[str]]=['eL.pL', 'eL.pR', 'eR.pL', 'eR.pR']) -> list[str]:
    """Gets the full paths to mc-opt-3 sample files

    Args:
        processes (Optional[Union[str,list[str]]], optional): sub-folders to look in. if None, default_processes(). Defaults to None.
        debug (bool, optional): if True, only one file per process (and filter, if given) will be output. Defaults to False.
        groups (Optional[list[str]], optional): logically disjoint filter sets. Defaults to ['eL.pL', 'eL.pR', 'eR.pL', 'eR.pR'].

    Returns:
        list[str]: _description_
    """
    
    if processes is None:
        processes = default_processes
    elif isinstance(processes, str):
        processes = [processes]
    
    arr = []
    for process in processes:
        carr = glob(f"/pnfs/desy.de/ilc/prod/ilc/ild/copy/dst-merged/500-TDR_ws/{process}/ILD_l5_o1_v02/**/*.slcio", recursive=True)
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
