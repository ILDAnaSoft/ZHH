import json, os
import numpy as np
from glob import glob
from dateutil import parser
from typing import Union, Optional

def get_dirs(path:str):
    return [ f.path for f in os.scandir(path) if f.is_dir() ]

def evaluate_runtime(DATA_ROOT:str,
                     bname:str,
                     WITH_EXIT_STATUS:bool=False):
    """_summary_

    Args:
        DATA_ROOT (str): _description_
        bname (str): base name of the result
        WITH_EXIT_STATUS (bool, optional): Requires --transfer-logs to be used when the law command is run. Defaults to False.

    Returns:
        _type_: _description_
    """
    
    branch = int(bname.split('-')[-1])
    
    with open(f'{DATA_ROOT}/{bname}/Source.txt') as file:
        src_path = file.read().strip()
        
    meta_file = glob(f'{DATA_ROOT}/{bname}/zhh*FinalStateMeta.json')
    
    assert(len(meta_file) == 1)
    with open(meta_file[0]) as metafile:
        branch_meta = json.load(metafile)
        n_proc, process = branch_meta['nEvtSum'], branch_meta['processName']
        tEnd, tStart = branch_meta['tEnd'], branch_meta['tStart']
    
    value = -1
    if WITH_EXIT_STATUS:
        with open(f'{DATA_ROOT}/stdall_{branch}To{branch+1}.txt') as file:
            marker = 'job exit code :'
            
            for line in file.readlines():
                if line.startswith(marker):
                    value = int(line.split(f'{marker} ')[1].strip())
            
    return (branch, process, n_proc, src_path, tEnd - tStart, tStart, tEnd, value)

def get_runtime_analysis(DATA_ROOT:Optional[str]=None,
                         chunks_factual:Optional[np.ndarray]=None,
                         meta:Optional[dict]=None,
                         WITH_EXIT_STATUS:bool=False)->np.ndarray:
    """_summary_

    Args:
        DATA_ROOT (Optional[str]): _description_
        meta (Optional[dict]): _description_

    Returns:
        np.ndarray: _description_
    """
    
    if chunks_factual is None and DATA_ROOT is None:
        raise Exception('Either chunks_factual or DATA_ROOT must be given')
    
    dtype = [
        ('branch', 'i'),
        ('process', '<U60'),
        ('n_processed', 'i'),
        ('src', '<U512'),
        ('tDuration', 'f')]
    
    results = []
    
    if chunks_factual is not None:
        results = chunks_factual[['branch', 'process', 'chunk_size_factual', 'location', 'runtime']].tolist()
    elif DATA_ROOT is not None:
        if meta is None:    
            metafile = glob(f'{DATA_ROOT}/htcondor_jobs*.json')[-1]
            
            with open(metafile) as file:
                meta = json.load(file)
        
        jobs = meta['jobs']
        dirs = get_dirs(DATA_ROOT)
    
        dtype += [('tStart', 'f')]
        dtype += [('tEnd', 'f')]
        dtype += [('exitCode', 'i')]
        
        for job_key in jobs:
            branch = jobs[job_key]['branches'][0]
            #if jobs[job_key]['status'] == 'finished':
            
            dir = list(filter(lambda a: a.endswith('-' + str(branch)), dirs))
            assert(len(dir) == 1)
            
            ev = evaluate_runtime(DATA_ROOT=DATA_ROOT, bname=os.path.basename(dir[0]), WITH_EXIT_STATUS=WITH_EXIT_STATUS)
            results.append(ev)
    else:
        raise Exception('No data source given')
    
    results = np.array(results, dtype=dtype)
                
    return results

def get_adjusted_time_per_event(runtime_analysis:np.ndarray,
                                 MAX_CAP:Optional[float]=None,
                                 MIN_CAP:Optional[float]=0.01)->np.ndarray:
    
    """Average for each process (i.e. over each polarization) the processing
    runtime and apply the MIN/MAX_CAP values.

    Returns:
        np.ndarray: A named numpy array containing the columns
            process, tAvg, tMax, n_processes and tPE (time per event)
    """
    
    unique_processes = np.unique(runtime_analysis['process'])

    dtype = [
        ('process', '<U64'),
        ('tAvg', 'f'),
        ('tMax', 'f'),
        ('n_processed', 'i'),
        ('tPE', 'f')]

    results = np.zeros(len(unique_processes), dtype=dtype)

    for i, process in enumerate(unique_processes):
        # Average for
        subset = runtime_analysis[runtime_analysis['process'] == process]

        tAvg = np.average(subset['tDuration'])
        i_max = np.argmax(subset['tDuration'])
        tMax = subset['tDuration'][i_max]
        n_processed = subset['n_processed'].sum()
        tPE = tMax/subset['n_processed'][i_max] #subset['tDuration'].sum()/ n_processed
        
        results['process'][i] = process
        results['tAvg'][i] = tAvg
        results['tMax'][i] = tMax
        results['n_processed'][i] = n_processed
        results['tPE'][i] = tPE
        
    if MAX_CAP is not None:
        results['tPE'][results['tPE'] > MIN_CAP] = np.minimum(results['tPE'], MAX_CAP)
        
    if MIN_CAP is not None:
        results['tPE'][results['tPE'] < MIN_CAP] = MIN_CAP
    
    return results