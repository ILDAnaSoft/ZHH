from glob import glob
from dateutil import parser
from typing import Optional, Union
import json
import numpy as np

DEFAULTS = {
    'PROD_NAME': '500-TDR_ws',
    'ILD_VERSION': 'ILD_l5_o1_v02'
}

def get_preselection_meta(DATA_ROOT:str)->dict:
    metafile = glob(f'{DATA_ROOT}/htcondor_jobs*.json')[0]
    with open(metafile) as file:
        meta = json.load(file)
        
    return meta

def file_get_polarization(src_path:str)->tuple[int, int]:
    pol_e = -1 if '.eL.p' in src_path else (+1 if '.eR.p' in src_path else 0)
    pol_p = -1 if '.pL.' in src_path else (+1 if '.pR.' in src_path else 0)
    
    if pol_e == 0 or pol_p == 0:
        print(f'Warning: Encountered 0 polarization (?)')
        
    return pol_e, pol_p

def parse_sample_path(src_path:str,
                    PROD_NAME:str=DEFAULTS['PROD_NAME'],
                    ILD_VERSION:str=DEFAULTS['ILD_VERSION'])->tuple:
    loc = src_path.split(f'/{PROD_NAME}/')[1]
    loc = loc.split(f'/{ILD_VERSION}/')[0]
    polarization = file_get_polarization(src_path)
    
    return (loc, polarization)

def get_preselection_summary_for_branch(
            DATA_ROOT:str,
            branch:Union[int,str],
            PROD_NAME:str=DEFAULTS['PROD_NAME'],
            ILD_VERSION:str=DEFAULTS['ILD_VERSION']):
    
    branch = int(branch)
    
    try:
        with open(f'{DATA_ROOT}/{branch}_Source.txt') as file:
            src_path = file.read().strip()            
    except:
        src_path = ''
    
    try: 
        with open(f'{DATA_ROOT}/{branch}_FinalStateMeta.json') as jsonfile:
            fs_meta = json.load(jsonfile)
            process = fs_meta['processName']  
    except:
        process = ''
    
    try:
        with open(f'{DATA_ROOT}/stdall_{branch}To{branch+1}.txt') as file:
            signals = ['start time    :', 'end time      :', 'job exit code :']
            values = [0, 0, 0]
            lsig = len(signals)
            
            for line in file.readlines():
                for i in range(lsig):
                    if line.startswith(signals[i]):
                        values[i] = line.split(f'{signals[i]} ')[1].strip()
                    elif src_path == '' and '--global.LCIOInputFiles=' in line:
                        src_path = line.split('--global.LCIOInputFiles=')[1].strip().split(' --constant.OutputDirectory=')[0]

            for i in [0, 1]:
                if values[i] != '':
                    if ' (' in values[i]:
                        values[i] = values[i].split(' (')[0]
                    
                    values[i] = float(parser.parse(values[i]).timestamp())
    except:
        values = [0, 0, 0]
    
    loc = ''
    polarization = (0, 0)
    if src_path != '':
        loc, polarization = parse_sample_path(src_path, PROD_NAME=PROD_NAME, ILD_VERSION=ILD_VERSION)
            
    return (branch, loc, process, polarization[0], polarization[1], src_path, values[0], values[1], values[1] - values[0], int(values[2]))

def get_preselection_summary(DATA_ROOT:str, meta:dict)->np.ndarray:
    """_summary_

    Args:
        meta (dict): result returned from get_preselection_meta()

    Returns:
        np.ndarray: _description_
    """
    
    jobs = meta['jobs']
    dtype = [
        ('status', '<U16'),
        ('branch', 'i'),
        ('loc', '<U32'),
        ('process', '<U32'),
        ('pol_e', 'i'),
        ('pol_p', 'i'),
        ('src', '<U255'),
        ('tStart', 'f'),
        ('tEnd', 'f'),
        ('tDuration', 'f'),
        ('exitCode', 'i')]
    
    results = np.empty(0, dtype=dtype)

    for job_key in jobs:
        branch = jobs[job_key]['branches'][0]
        status = jobs[job_key]['status']
        
        ev = get_preselection_summary_for_branch(DATA_ROOT, branch)
        entry = (status, *ev)

        results = np.append(results, np.array([entry], dtype=dtype))
    
    return results