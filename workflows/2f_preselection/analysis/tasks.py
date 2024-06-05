# coding: utf-8

from zhh import get_raw_files
import law
import luigi

# import our "framework" tasks
from analysis.framework import HTCondorWorkflow
from phc.tasks import ShellTask
import os.path as osp

class Preselection(ShellTask, HTCondorWorkflow, law.LocalWorkflow):
    debug = luigi.BoolParameter(default=False)
    
    def create_branch_map(self) -> dict[int, str]:
        arr = get_raw_files()
        
        # for debugging: only first three entries
        if self.debug:
            arr = arr[:3]
        
        res = { k: v for k, v in zip(list(range(len(arr))), arr) }
        
        return res

    def output(self):
        return self.target_collection([
            self.local_target(f'{self.branch}/zhh_FinalStates.root'),
            self.local_target(f'{self.branch}/zhh_Preselection.root')
        ])
        #return self.local_target(f'{self.branch}/zhh_FinalStates.root')

    def build_command(self, fallback_level):
        output_root = osp.dirname(str(self.output()[0].path))
        
        cmd =  f'source /afs/desy.de/user/b/bliewert/public/MarlinWorkdirs/ZHH/setup.sh'
        cmd += f' && mkdir -p output'
        cmd += f' && Marlin $REPO_ROOT/scripts/newZHHllbbbb.xml {"" if (self.debug == True) else "--global.MaxRecordNumber=0 "}--global.LCIOInputFiles={self.branch_map[self.branch]}'
        cmd += f' && mkdir -p {output_root} && mv output/* {output_root}'

        return cmd

