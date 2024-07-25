import json
import numpy as np
from glob import glob
from dateutil import parser
from typing import Union, Optional

def evaluate_runtime(DATA_ROOT:str,
                     branch:Union[int,str],
                     MARLIN_STARTUP_TIME:int=15):
    branch = int(branch)
    
    with open(f'{DATA_ROOT}/{branch}_Source.txt') as file:
        src_path = file.read().strip()
        
    with open(f'{DATA_ROOT}/{branch}_FinalStateMeta.json') as metafile:
        branch_meta = json.load(metafile)
        n_proc, process = branch_meta['nEvtSum'], branch_meta['processName']
        
    with open(f'{DATA_ROOT}/stdall_{branch}To{branch+1}.txt') as file:
        signals = ['start time    :', 'end time      :', 'job exit code :']
        values = ['', '', '']
        lsig = len(signals)
        
        for line in file.readlines():
            for i in range(lsig):
                if line.startswith(signals[i]):
                    values[i] = line.split(f'{signals[i]} ')[1].strip()
                    
        for i in [0, 1]:
            if values[i] != '':
                if ' (' in values[i]:
                    values[i] = values[i].split(' (')[0]
                
                values[i] = float(parser.parse(values[i]).timestamp())
            
    return (branch, process, n_proc, src_path, values[0], values[1], values[1] - values[0] - MARLIN_STARTUP_TIME, int(values[2]))

def get_runtime_analysis(DATA_ROOT:Optional[str]=None,
                         meta:Optional[dict]=None)->np.ndarray:
    """_summary_

    Args:
        meta (Optional[dict]): _description_
        DATA_ROOT (Optional[str]): _description_

    Returns:
        np.ndarray: _description_
    """
    
    if meta is None and DATA_ROOT is None:
        raise Exception('Either meta or DATA_ROOT must be given')
    
    if meta is None:    
        metafile = glob(f'{DATA_ROOT}/htcondor_jobs*.json')[0]
        with open(metafile) as file:
            meta = json.load(file)
    
    jobs = meta['jobs']
    dtype = [
        ('branch', 'i'),
        ('process', '<U60'),
        ('n_processed', 'i'),
        ('src', '<U512'),
        ('tStart', 'f'),
        ('tEnd', 'f'),
        ('tDuration', 'f'),
        ('exitCode', 'i')]
    
    results = np.empty(0, dtype=dtype)

    for job_key in jobs:
        branch = jobs[job_key]['branches'][0]
        if jobs[job_key]['status'] == 'finished':
            ev = evaluate_runtime(DATA_ROOT=DATA_ROOT, branch=branch)
            results = np.append(results, np.array([ev], dtype=dtype))
    
    return results