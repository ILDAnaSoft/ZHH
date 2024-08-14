import json
import numpy as np
from glob import glob
from dateutil import parser
from typing import Union, Optional

def evaluate_runtime(DATA_ROOT:str,
                     branch:Union[int,str]):
    branch = int(branch)
    
    with open(f'{DATA_ROOT}/{branch}_Source.txt') as file:
        src_path = file.read().strip()
        
    with open(f'{DATA_ROOT}/{branch}_FinalStateMeta.json') as metafile:
        branch_meta = json.load(metafile)
        n_proc, process = branch_meta['nEvtSum'], branch_meta['processName']
        tEnd, tStart = branch_meta['tEnd'], branch_meta['tStart']
        
    with open(f'{DATA_ROOT}/stdall_{branch}To{branch+1}.txt') as file:
        marker = 'job exit code :'
        value = ''
        
        for line in file.readlines():
            if line.startswith(marker):
                value = line.split(f'{marker} ')[1].strip()
            
    return (branch, process, n_proc, src_path, tStart, tEnd, tEnd - tStart, int(value))

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

def get_adjusted_time_per_event(runtime_analysis:np.ndarray,
                                 normalize_by_min:bool=True,
                                 MAX_CAP:Optional[float]=None,
                                 MIN_CAP:Optional[float]=None):
    unique_processes = np.unique(runtime_analysis['process'])

    dtype = [
        ('process', '<U64'),
        ('tAvg', 'f'),
        ('n_processed', 'i'),
        ('tPE', 'f')]

    results = np.empty(len(unique_processes), dtype=dtype)

    i = 0
    for process in unique_processes:
        # Average for
        mask = runtime_analysis['process'] == process
        tAvg = np.average(runtime_analysis['tDuration'][mask])
        n_processed = int(np.average(runtime_analysis['n_processed'][mask])) # should be equal
        tPE = tAvg/n_processed
        
        results[i] = np.array([ (process, tAvg, n_processed, tPE) ], dtype=dtype)
        i += 1
        
    if normalize_by_min:
        results['tPE'] *= 1/results['tPE'].min()
        
    if MAX_CAP is not None:
        results['tPE'] = np.minimum(results['tPE'], MAX_CAP)
        
    if MIN_CAP is not None:
        results['tPE'][results['tPE'] < MIN_CAP] = 1
        
    results['tPE'][results['tPE'] < 1] = 1
    
    return results