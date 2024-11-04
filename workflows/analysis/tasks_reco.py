from .tasks_abstract import FastSimSGVExternalReadJob
from analysis.framework import zhh_configs
from glob import glob
import os.path as osp

class FastSimSGV(FastSimSGVExternalReadJob):    
    @property
    def sgv_input_files(self)->list[str]:
        if not '__sgv_input_files' in self:
            raise ValueError('No SGV input files attached')
        
        return self.__sgv_input_files

    @sgv_input_files.setter
    def sgv_input_files(self, input_files:list[str]):
        self.__sgv_input_files = input_files
        
    def create_branch_map(self):
        files = self.sgv_input_files
        files.sort()

        return { k: v for k, v in zip(list(range(len(files))), files) }
        
    def output(self):
        return self.local_target(osp.basename(str(self.branch_map[self.branch])))