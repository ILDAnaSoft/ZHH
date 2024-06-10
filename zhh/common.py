from glob import glob

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

def get_raw_files(process:str = '2f_Z_hadronic') -> list[str]:
    arr = glob(f"/pnfs/desy.de/ilc/prod/ilc/mc-2020/ild/dst-merged/500-TDR_ws/{process}/ILD_l5_o1_v02/v02-02-03/*/*/*.slcio")
    arr.sort()
    
    return arr


