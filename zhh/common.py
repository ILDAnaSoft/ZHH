from glob import glob

def get_raw_files(process:str = '2f_Z_hadronic') -> list[str]:
    arr = glob(f"/pnfs/desy.de/ilc/prod/ilc/mc-2020/ild/dst-merged/500-TDR_ws/{process}/ILD_l5_o1_v02/v02-02-03/*/*/*.slcio")
    arr.sort()
    
    return arr


