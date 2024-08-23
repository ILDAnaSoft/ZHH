import numpy as np
from math import floor, ceil
from typing import Optional
from .PreselectionAnalysis import sample_weight

def get_process_normalization(
        processes:np.ndarray,
        samples:np.ndarray,
        RATIO_BY_EXPECT:float=1.):
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
        
        ('n_events_tot', 'l'),
        ('n_events_expected', 'f'),
        ('n_events_normalized', 'l')]
    
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
    
    #norm_by = results['n_events_tot'] / results['n_events_expected']
    #bound_val = np.min(norm_by)*RATIO_BY_EXPECT
    
    #print(f'Normalizing by proc_pol {results["proc_pol"][np.argmin(norm_by)]}')
    #print(f'bound_val {bound_val}')
    #print(results['event_weight'])
    
    results['n_events_normalized'] = np.minimum(results['n_events_tot'], np.ceil(RATIO_BY_EXPECT * results['n_events_expected']))
    
    assert(np.sum(results['n_events_normalized'] < 0) == 0)
    
    return results

def get_sample_chunk_splits(
        samples:np.ndarray,
        adjusted_time_per_event:Optional[np.ndarray]=None,
        process_normalization:Optional[np.ndarray]=None,
        MAXIMUM_TIME_PER_JOB:int=5400
        ):
    """_summary_

    Args:
        samples (np.ndarray): _description_
        adjusted_time_per_event (Optional[np.ndarray], optional): _description_. Defaults to None.
        process_normalization (Optional[np.ndarray], optional): _description_. Defaults to None.
        MAXIMUM_TIME_PER_JOB (int, optional): For splitting jobs, in seconds. Defaults to 5400 (1.5h).

    Returns:
        _type_: _description_
    """
    
    dtype = [
        ('branch', 'i'),
        ('process', '<U60'),
        ('proc_pol', '<U64'),
        ('location', '<U512'),
        
        ('n_chunks', 'i'),
        ('chunk_start', 'i'),
        ('chunk_size', 'i'),
    ]

    results = np.empty(0, dtype=dtype)
    
    if process_normalization is not None:
        pn = process_normalization
        atpe = adjusted_time_per_event
        
        for p in pn:
            n_target = p['n_events_normalized']
            
            if n_target > 0:
                n_accounted = 0
                
                c_chunks = []
                n_chunks = 0
                
                c_samples = samples[samples['proc_pol'] == p['proc_pol']]
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
                        c_chunks.append((0, p['process'], p['proc_pol'], sample['location'], n_chunks, n_accounted_sample, c_chunk_size))
                        
                        n_accounted += c_chunk_size
                        n_accounted_sample += c_chunk_size
        
                        n_chunks += 1
                        
                    n_sample += 1
                        
                results = np.append(results, np.array(c_chunks, dtype=dtype))
        
    elif adjusted_time_per_event is not None:
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
                    (0, s['process'], s['proc_pol'], s['location'], c_chunk_size, n_chunks, chunk_start)
                ], dtype=dtype))
                chunk_start = n_accounted
    
    results['branch'] = np.arange(len(results))
    
    return results