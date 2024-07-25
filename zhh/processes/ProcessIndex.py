#import sqlite3 as sqlite
from glob import glob
from typing import Optional
import json, os, csv
import os.path as osp
import numpy as np

from tqdm.auto import tqdm
from zhh.util import is_readable
from zhh.analysis import get_pol_key, parse_sample_path

class ProcessIndex:
    process_index: str = 'processes.json'
    samples_index: str = 'samples.json'
    
    dtype_sample = [
        ('run_id', 'i'),
        ('process', '<U60'),
        ('proc_pol', '<U64'),
        ('n_events', 'i'),
        ('pol_e', 'i'),
        ('pol_p', 'i'),
        ('location', '<U512'),]

    dtype_process = [
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
             CHUNK_SIZE:int=0,
             pbar:bool=True):
        
        from pyLCIO import IOIMPL
        
        remaining_files = list(set(self.RAW_FILE_LIST) - set(self.samples['location']))

        if CHUNK_SIZE > 0:
            ci = 0
            chunk = []

            while ci < len(remaining_files):
                file = remaining_files[ci]
                ci += 1
                if is_readable(file):
                    chunk.append(file)
                    
                    if len(chunk) == CHUNK_SIZE:
                        break
            remaining_files = chunk                
        else:
            remaining_files = list(filter(is_readable, remaining_files))

        for location in (progress := tqdm(remaining_files, disable=not pbar)):
            progress.set_description(f'Reading file {location}')
            
            reader = IOIMPL.LCFactory.getInstance().createLCReader()
            reader.open(location)
            
            event = reader.readNextEvent()
            params = event.getParameters()
            
            file_meta = {
                'process': params.getStringVal('processName'),
                'nEvtSum': reader.getNumberOfEvents(),
                'run': event.getRunNumber(),
                'beamPol1': params.getFloatVal('Pol0'),
                'beamPol2': params.getFloatVal('Pol1'),
                'beamPol1Alt': params.getFloatVal('beamPol1'),
                'beamPol2Alt': params.getFloatVal('beamPol2'),
                'crossSection': params.getFloatVal('crossSection'),
                'crossSection_err': params.getFloatVal('crossSectionError'),
                'process_id': params.getIntVal('ProcessID'),
            }
            reader.close()
            
            if file_meta['beamPol1'] != 0 or file_meta['beamPol2'] != 0:
                pol_em, pol_ep = file_meta['beamPol1'], file_meta['beamPol2']
            else:
                pol_em, pol_ep = file_meta['beamPol1Alt'], file_meta['beamPol2Alt']
                
            process = file_meta['process']
            
            proc_pol = f'{process}_{get_pol_key(pol_em, pol_ep)}'
            
            if not proc_pol in self.processes['proc_pol']:
                cx, cx_err = file_meta['crossSection'], file_meta['crossSection_err']
                self.processes = np.append(self.processes, [np.array([
                    (process, proc_pol, pol_em, pol_ep, cx, cx_err, file_meta['process_id'])
                ], dtype=self.dtype_process)])
                
            run_id = file_meta['run']
            if not location in self.samples['location']:
                self.samples = np.append(self.samples, [np.array([
                    (run_id, process, proc_pol, file_meta['nEvtSum'], pol_em, pol_ep, location)
                ], dtype=self.dtype_sample)])
                
            self.save()
        
        self.STATE = 1
        progress.set_description('Finished indexing processes and samples')

    def save(self):
        np.save(self.SAMPLE_INDEX, self.samples)
        np.save(self.PROCESS_INDEX, self.processes)