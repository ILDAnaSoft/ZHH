from .tasks_abstract import FastSimSGVExternalReadJob
from glob import glob
import os.path as osp

class FastSimSGV(FastSimSGVExternalReadJob):    
    def create_branch_map(self):
        if str(self.version).startswith('550-hh-fast'):
            files = glob('/pnfs/desy.de/ilc/prod/ilc/mc-2020/generated/550-Test/hh/*.slcio')
            files.sort()
        else:
            raise Exception('Cannot process version/source type <{self.version}>')
        
        return { k: v for k, v in zip(list(range(len(files))), files) }
        
    def output(self):
        return self.local_target(osp.basename(str(self.branch_map[self.branch])))