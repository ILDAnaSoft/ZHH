#import sqlite3 as sqlite
from glob import glob
from typing import Optional, List
import json, os, csv, time
import os.path as osp
import numpy as np

from tqdm.auto import tqdm
from zhh.util import is_readable
from zhh.analysis import get_pol_key, parse_sample_path
from multiprocessing import Process, Queue

class SampleMeta():   
    def __init__(self, process:str, n_events:int, run_number:int, \
                beamPol1:float, beamPol2:float, \
                beamPol1Alt:float, beamPol2Alt:float, \
                crossSection:float, crossSection_err:float, \
                process_id:int):
        
        self.process = process
        self.nEvtSum = n_events
        self.run = run_number
        self.beamPol1 = beamPol1
        self.beamPol2 = beamPol2
        self.beamPol1Alt = beamPol1Alt
        self.beamPol2Alt = beamPol2Alt
        self.crossSection = crossSection
        self.crossSection_err = crossSection_err
        self.process_id = process_id
        
    @classmethod
    def fromevent(cls, params, n_events:int, run_number:int):
        "Initialize SampleMeta from LCIO event data"
        
        return cls(params.getStringVal('processName'), n_events, run_number, \
                   params.getFloatVal('Pol0'), params.getFloatVal('Pol1'), \
                   params.getFloatVal('beamPol1'), params.getFloatVal('beamPol2'), \
                   params.getFloatVal('crossSection'), params.getFloatVal('crossSectionError'), \
                   params.getIntVal('ProcessID'))

def per_chunk(q:Queue, file_paths:List[str]):
    from pyLCIO import IOIMPL
    for location in (pbar := tqdm(file_paths)):
        pbar.set_description(f'Processing file {location}')
        
        reader = IOIMPL.LCFactory.getInstance().createLCReader()
        reader.open(location)
        
        event = reader.readNextEvent()
        params = event.getParameters()
        
        file_meta = SampleMeta.fromevent(params, reader.getNumberOfEvents(), event.getRunNumber())
        
        q.put(file_meta)

class ProcessIndex:
    process_index: str = 'processes.json'
    samples_index: str = 'samples.json'
    
    dtype_sample = [
        ('sid', 'i'),
        ('run_id', 'i'),
        ('process', '<U60'),
        ('proc_pol', '<U64'),
        ('n_events', 'i'),
        ('pol_e', 'i'),
        ('pol_p', 'i'),
        ('location', '<U512'),]

    dtype_process = [
        ('pid', 'i'),
        ('process', '<U60'),
        ('proc_pol', '<U64'),
        ('pol_e', 'i'),
        ('pol_p', 'i'),
        
        ('cross_sec', 'f'),
        ('cross_sec_err', 'f'),
        ('generator_id', 'i')]
    
    def __init__(self,
                 PROCESS_INDEX:str,
                 SAMPLE_INDEX:str,
                 RAW_FILE_LIST:list[str]):
        
        self.PROCESS_INDEX = PROCESS_INDEX
        self.SAMPLE_INDEX = SAMPLE_INDEX
        self.RAW_FILE_LIST = RAW_FILE_LIST
        self.STATE = 0
        
        if osp.isfile(PROCESS_INDEX) and osp.isfile(SAMPLE_INDEX):
            self.processes = np.load(PROCESS_INDEX)
            self.samples = np.load(SAMPLE_INDEX)
        else:
            self.samples = np.empty(0, dtype=self.dtype_sample)
            self.processes = np.empty(0, dtype=self.dtype_process)
        
    def load(self,
             CHUNK_SIZE:int=128,
             pbar:bool=True):
        
        remaining_files = list(set(self.RAW_FILE_LIST) - set(self.samples['location']))
        
        files = []
        if CHUNK_SIZE == 0:
            files.append(remaining_files)
        else:
            for i in range(0, len(remaining_files), CHUNK_SIZE):
                files.append(remaining_files[i:i+CHUNK_SIZE])
        
        n_process = 0
        n_sample = 0
        
        for i, chunk in enumerate((progress := tqdm(files, disable=not pbar))):
            progress.set_description(f'Indexing chunk {i}')
            
            q = Queue(len(chunk))
            p = Process(target=per_chunk, args=(q, chunk))
            p.start()
            while not q.full():
                time.sleep(0.1)
            
            p.terminate()
            p.join()
            
            j = 0
            while not q.empty():
                meta:SampleMeta = q.get()
                location = chunk[j]
                
                if meta.beamPol1 != 0 or meta.beamPol2 != 0:
                    pol_em, pol_ep = meta.beamPol1, meta.beamPol2
                else:
                    pol_em, pol_ep = meta.beamPol1Alt, meta.beamPol2Alt
                    
                process = meta.process
                
                proc_pol = f'{process}_{get_pol_key(pol_em, pol_ep)}'
                
                if not proc_pol in self.processes['proc_pol']:
                    cx, cx_err = meta.crossSection, meta.crossSection_err
                    self.processes = np.append(self.processes, [np.array([
                        (n_process, process, proc_pol, pol_em, pol_ep, cx, cx_err, meta.process_id)
                    ], dtype=self.dtype_process)])
                    n_process += 1
                    
                if not location in self.samples['location']:
                    self.samples = np.append(self.samples, [np.array([
                        (n_sample, meta.run, process, proc_pol, meta.nEvtSum, pol_em, pol_ep, location)
                    ], dtype=self.dtype_sample)])
                    n_sample += 1
                    
                j += 1
                    
            self.save()
        
        self.STATE = 1
        progress.set_description('Finished indexing processes and samples')

    def save(self):
        np.save(self.SAMPLE_INDEX, self.samples)
        np.save(self.PROCESS_INDEX, self.processes)