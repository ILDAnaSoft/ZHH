from .tasks_abstract import FastSimSGVExternalReadJob
from glob import glob
import os.path as osp

class SGV550hh(FastSimSGVExternalReadJob):
    def create_branch_map(self):
        files = glob('/pnfs/desy.de/ilc/prod/ilc/mc-2020/generated/550-Test/hh/*.slcio')
        files.sort()
        
        return { k: v for k, v in zip(list(range(len(files))), files) }
        
    def output(self):
        return self.local_target(osp.basename(str(self.branch_map[self.branch])))