import numpy as np
from math import floor, ceil

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