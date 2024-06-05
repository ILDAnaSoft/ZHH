# coding: utf-8

from zhh import get_raw_files
import law

# import our "framework" tasks
from analysis.framework import HTCondorWorkflow
from phc.tasks import ShellTask
import os.path as osp

class Preselection(ShellTask, HTCondorWorkflow, law.LocalWorkflow):
    def create_branch_map(self) -> dict[int, str]:
        # map branch indexes to input files
        arr = get_raw_files()
        
        # as test: only first three entries
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
        
        print(f'Output Root: {output_root}')
        
        cmd =  f'source /afs/desy.de/user/b/bliewert/public/MarlinWorkdirs/ZHH/setup.sh'
        cmd += f' && mkdir -p output'
        cmd += f' && Marlin $REPO_ROOT/scripts/newZHHllbbbb.xml --global.MaxRecordNumber=100 --global.LCIOInputFiles={self.branch_map[self.branch]}'
        cmd += f' && mkdir -p {output_root} && mv output/* {output_root}'
        
        #self.output().path

        return cmd

