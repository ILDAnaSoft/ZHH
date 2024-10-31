import json
import numpy as np
import uproot as ur
from math import floor, ceil
from typing import Optional, List, Iterable
from .PreselectionAnalysis import sample_weight

def get_process_normalization(
        processes:np.ndarray,
        samples:np.ndarray,
        RATIO_BY_EXPECT:Optional[float]=1.):
    """Returns a np.ndarray with

    Args:
        processes (np.ndarray): _description_
        samples (np.ndarray): _description_
        RATIO_BY_EXPECT (Optional[float], optional): If None or 0, will use all the data. Defaults to 1..

    Returns:
        _type_: _description_
    """
    
    dtype = [
        ('process', '<U60'),
        ('proc_pol', '<U64'),
        ('cross_sec', 'f'),
        ('event_weight', 'f'),
        
        ('n_events_tot', 'l'),
        ('n_events_expected', 'f'),
        ('n_events_target', 'l')]
    
    results = np.empty(0, dtype=dtype)
    
    # Find the least represented proc_pol combination
    # First find n_events_tot for each proc_pol
    for p in processes:
        n_events_tot = 0
        process, proc_pol, cross_sec = p['process'], p['proc_pol'], p['cross_sec']
        pol = p['pol_e'], p['pol_p']
        
        for s in samples[samples['process'] == process]:
            n_events_tot += s['n_events']
            
        n_events_expected = sample_weight(cross_sec, pol, n_gen=1)
        event_weight = n_events_expected / n_events_tot
            
        results = np.append(results, np.array([
            (process, proc_pol, cross_sec, event_weight, n_events_tot, n_events_expected, 0)
        ], dtype=dtype))
        
    # Normalize by cross-section
    results = results[np.argsort(results['proc_pol'])]
    if RATIO_BY_EXPECT is None or RATIO_BY_EXPECT == 0:
        results['n_events_target'] = results['n_events_tot']
    else:
        results['n_events_target'] = np.maximum(
            np.minimum(results['n_events_tot'], np.ceil(RATIO_BY_EXPECT * results['n_events_expected'])),
            50*np.ones(len(results), dtype=int)
        )
    
    assert(np.sum(results['n_events_target'] < 0) == 0)
    
    return results

def get_sample_chunk_splits(
        samples:np.ndarray,
        adjusted_time_per_event:np.ndarray,
        process_normalization:np.ndarray,
        custom_statistics:Optional[List[tuple]]=None,
        existing_chunks:Optional[np.ndarray]=None,
        MAXIMUM_TIME_PER_JOB:int=5400,
        ):
    """_summary_

    Args:
        samples (np.ndarray): _description_
        adjusted_time_per_event (np.ndarray): _description_. Defaults to None.
        process_normalization (np.ndarray): _description_.
        custom_statistics (Optional[List[tuple]], optional): list of entries of either (fraction:float, processes:list[str]) or
            (fraction:float, processes:list[str], reference:str<'total', 'expected'>)
        existing_chunks (Optional[np.ndarray], optional): _description_. Defaults to None.
        MAXIMUM_TIME_PER_JOB (int, optional): For splitting jobs, in seconds. Defaults to 5400 (1.5h).

    Returns:
        _type_: _description_
    """
    
    dtype = [
        ('branch', 'I'),
        ('sid', 'I'),
        ('process', '<U60'),
        ('proc_pol', '<U64'),
        ('location', '<U512'),
        
        ('n_chunks', 'I'),
        ('chunk_start', 'I'),
        ('chunk_size', 'I'),
    ]

    results = np.empty(0, dtype=dtype) if existing_chunks is None else np.copy(existing_chunks)
    
    pn = process_normalization
    atpe = adjusted_time_per_event
    
    if isinstance(custom_statistics, Iterable):
        for entry in custom_statistics:
            if len(entry) == 2:
                fraction, processes = entry
                reference = 'total'
            elif len(entry) == 3:
                fraction, processes, reference = entry
                reference = reference.lower()
            else:
                raise Exception('Cannot interpret custom_statistics')
            
            processes = np.unique(processes)
            mask = np.isin(pn['process'], processes)
            
            pn['n_events_target'][mask] = np.ceil(fraction*pn['n_events_' + ('tot' if reference == 'total' else 'expected')][mask])
            pn['n_events_target'][mask] = np.minimum(pn['n_events_target'][mask], pn['n_events_tot'][mask])
    
    n_chunks_tot = 0 if existing_chunks is None else len(existing_chunks)
    
    for p in pn:
        n_target = p['n_events_target']
        
        if n_target > 0:
            c_chunks = []
            c_samples = samples[samples['proc_pol'] == p['proc_pol']]
            
            if existing_chunks is not None:
                mask = existing_chunks['proc_pol'] == p['proc_pol']
                
                n_accounted = np.sum(existing_chunks['chunk_size'][mask])
                n_chunks = np.max(existing_chunks['n_chunks'][mask]) + 1
                n_sample = len(np.unique(existing_chunks['location'][mask]))
            else:
                n_accounted = 0
                n_chunks = 0
                n_sample = 0
            
            while n_sample < len(c_samples) and n_accounted < n_target:
                sample = c_samples[n_sample]
                
                n_accounted_sample = 0
                n_tot_sample = sample['n_events']
                
                max_chunk_size = 99999
            
                if atpe is not None:
                    time_per_event = atpe['tPE'][atpe['process'] == p['process']]
                    max_chunk_size = floor(MAXIMUM_TIME_PER_JOB/time_per_event)
                    
                while n_accounted < n_target and n_accounted_sample < n_tot_sample:
                    c_chunk_size = min(min(n_tot_sample - n_accounted_sample, max_chunk_size), n_target - n_accounted)
                    c_chunks.append((n_chunks_tot, sample['sid'], p['process'], p['proc_pol'], sample['location'], n_chunks, n_accounted_sample, c_chunk_size))
                    
                    n_accounted += c_chunk_size
                    n_accounted_sample += c_chunk_size
    
                    n_chunks += 1
                    n_chunks_tot += 1
                    
                n_sample += 1
                    
            if len(c_chunks) > 0:
                results = np.append(results, np.array(c_chunks, dtype=dtype))
    
    return results

def get_chunks_factual(DATA_ROOT:str, chunks_in:np.ndarray, attach_time:bool=False):
    dtype_arr = chunks_in.dtype.descr + [('chunk_size_factual', 'I')]
    if attach_time:
        dtype_arr += [('runtime', 'I')]
    
    dtype_new = np.dtype(dtype_arr)
    
    chunks = np.zeros(chunks_in.shape, dtype_new)
    
    for name in chunks_in.dtype.names:
        chunks[name] = chunks_in[name]
    
    branches = chunks['branch']
    todelete = []
    for branch in branches:
        try:
            with open(f'{DATA_ROOT}/{branch}/zhh_FinalStateMeta.json') as jf:
                meta = json.load(jf)
                n_events = meta['nEvtSum']
                dt = meta['tEnd'] - meta['tStart']
                
            chunks['chunk_size_factual'][chunks['branch'] == branch] = n_events
            if attach_time:
                chunks['runtime'][chunks['branch'] == branch] = dt
        except:        
            try:
                if attach_time:
                    raise Exception('Not possible with attach_time=True')
                
                with ur.open(f'{DATA_ROOT}/{branch}/zhh_FinalStates.root') as rf:
                    n_events = len(rf['event'].array())
                    
                chunks['chunk_size_factual'][chunks['branch'] == branch] = n_events
            except:
                print(f'Skipping chunk {branch} (unrecoverable), will be removed')
                chunks['chunk_size_factual'][chunks['branch'] == branch] = 0
                todelete.append(branch)    
    
    if len(todelete):
        chunks = np.delete(chunks, todelete, axis=0)
    
    return chunks
            
            