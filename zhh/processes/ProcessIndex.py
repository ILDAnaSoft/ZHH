#import sqlite3 as sqlite
from glob import glob
from typing import Optional
import json, os, csv
import os.path as osp
import pandas as pd

from zhh.processes.DirectoryReader import read_meta_file
from zhh.util import df_append

class ProcessIndex:
    process_index: str = 'processes.csv'
    results_index: str = 'results.csv'
    
    columns_processes = ['processId', 'processName',
                         'polElectron', 'polPositron',
                         'crossSection', 'crossSectionError',
                         'nEventsTot']
    
    columns_results = ['runId', 'nEvents',
                       'fsMetaPath', 'fsPath',
                       'preselHHllPath', 'preselHHvvPath', 'preselHHqqPath']
    
    def __setitem__(self, key:str, value:pd.DataFrame):
        if key == 'processes':
            self.processes = value
        elif key == 'results':
            self.results = value
        else:
            raise KeyError(f'Unknown key <{key}>')
    
    def __init__(self,
                 root_path:str,
                 reindex:bool=True):
        
        print(f'asdfasdfasdf')
        
        # Load existing index
        self.processes = pd.DataFrame(columns=self.columns_processes)
        self.results = pd.DataFrame(columns=self.columns_results)
        
        process_path = osp.join(root_path, self.process_index)
        results_path = osp.join(root_path, self.results_index)
        
        for df, path in (('processes', process_path),
                         ('results', results_path)):
            
            if osp.isfile(path):
                if reindex:
                    os.remove(path)
                    
                self[df] = pd.read_csv(path)
        
        # Find all files
        meta_files = glob(root_path + '/*.json')
        
        for file in meta_files:
            print(file)
            process, result = read_meta_file(file)
            
            if not (process.process_id in self.processes['processId'].values):
                self.processes = df_append(self.processes, {
                    'processId': process.process_id,
                    'processName': process.process_name,
                    'polElectron': process.pol_electron,
                    'polPositron': process.pol_positron,
                    'crossSection': process.cross_section,
                    'crossSectionError': process.cross_section_err
                })
            else:
                self.processes['nEventsTot'][self.processes['processId'] == process.process_id] += result.n_events
            
            if not (result.run_id in self.results['runId'].values):
                self.results = df_append(self.results, {
                    'runId': result.run_id,
                    'nEvents': result.n_events,
                    'fsMetaPath': result.final_state_meta_path,
                    'fsPath': result.final_state_path,
                    'preselHHllPath': result.zhh_presel_llhh_path,
                    'preselHHvvPath': result.zhh_presel_vvhh_path,
                    'preselHHqqPath': result.zhh_presel_qqhh_path
                })
            
        # Save index
        self.processes.to_csv(process_path)
        self.results.to_csv(results_path)
        
a = ProcessIndex('/root/public/MarlinWorkdirs/ZHH/output')
