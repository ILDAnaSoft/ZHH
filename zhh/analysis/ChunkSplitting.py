import numpy as np
from math import floor, ceil
from .PreselectionAnalysis import sample_weight

def get_sample_count_normalization(processes:np.ndarray,
                                   samples:np.ndarray):
    """Returns a np.ndarray with

    Args:
        processes (np.ndarray): _description_
        samples (np.ndarray): _description_

    Returns:
        _type_: _description_
    """
    
    dtype = [
        ('process', '<U60'),
        ('proc_pol', '<U64'),
        ('cross_sec', 'f'),
        ('event_weight', 'f'),
        
        ('n_events_tot', 'i'),
        ('n_events_normalized', 'i')]
    
    results = np.empty(0, dtype=dtype)
    
    # Find the least least represented proc_pol combination
    # First find n_events_tot for each proc_pol
    for p in processes:
        n_events_tot = 0
        process, proc_pol, cross_sec = p['process'], p['proc_pol'], p['cross_sec']
        pol = p['pol_e'], p['pol_p']
        
        for s in samples[samples['process'] == process]:
            n_events_tot += s['n_events']
            
        event_weight = sample_weight(cross_sec, pol)
            
        results = np.append(results, np.array([
            (process, proc_pol, cross_sec, event_weight, n_events_tot, 0)
        ], dtype=dtype))
        
    # Normalize by cross-section
    frac = np.min(results['n_events_tot'] / results['event_weight'])
        
    
    return results

def get_sample_chunk_splits(samples:np.ndarray,
                      adjusted_time_per_event:np.ndarray):
    
    dtype = [
        ('process', '<U60'),
        ('proc_pol', '<U64'),
        ('location', '<U512'),
        
        ('chunk_size', 'i'),
        ('n_chunks', 'i'),
        ('chunk_start', 'i')]

    results = np.empty(0, dtype=dtype)
    
    for s in samples:
        weight = adjusted_time_per_event['tPE'][adjusted_time_per_event['process'] == s['process']]
        n_events = s['n_events']

        MIN_UNITS_PER_CHUNK = 8000 # one unit = one unit event * one unit of weight; see the process type for which tPE == 1.0
        
        chunk_size = max(1, floor(n_events / weight))
        n_chunks = max(1, ceil(n_events/chunk_size))

        if n_chunks > 1 and (n_events % n_chunks)*weight < MIN_UNITS_PER_CHUNK:
            n_chunks -= 1
            
        # skip:execute
        # 0:chunk_size-1
        # chunk_size:2xchunk_size -1
        
        n_accounted = 0
        chunk_start = 0
        for i in range(n_chunks):
            c_chunk_size = min(chunk_size, n_events - n_accounted)
            n_accounted += c_chunk_size
            
            assert(c_chunk_size > 0)
            
            results = np.append(results, np.array([
                (s['process'], s['proc_pol'], s['location'], c_chunk_size, n_chunks, chunk_start)
            ], dtype=dtype))
            chunk_start = n_accounted
        
    return results