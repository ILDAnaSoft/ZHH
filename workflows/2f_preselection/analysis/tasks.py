# coding: utf-8

from zhh import get_raw_files
import law

# import our "framework" tasks
from analysis.framework import HTCondorWorkflow
from phc.tasks import ShellTask
import os.path as osp

class Preselection(ShellTask, HTCondorWorkflow, law.LocalWorkflow):
    def create_branch_map(self) -> dict[int, str]:
        # map branch indexes to ascii numbers from 97 to 122 ("a" to "z")
        arr = get_raw_files()
        
        # as test: only first three entries
        arr = arr[:3]
        
        res = { k: v for k, v in zip(list(range(len(arr))), arr) }
        
        return res #{i: num for i, num in enumerate(range(97, 122 + 1))}

    def output(self):
        # it's best practice to encode the branch number into the output target
        return self.local_target(f'{self.branch}/zhh_FinalStates.root')

    def build_command(self, fallback_level):
        output_root = osp.dirname(str(self.output().path))
        
        cmd =  f'source /afs/desy.de/user/b/bliewert/public/MarlinWorkdirs/ZHH/setup.sh'
        cmd += f' && mkdir output'
        cmd += f' && Marlin $REPO_ROOT/scripts/newZHHllbbbb.xml --global.MaxRecordNumber=0 --global.LCIOInputFiles={self.branch_map[self.branch]}'
        cmd += f' && mv output {output_root}/{self.branch}'

        return cmd

