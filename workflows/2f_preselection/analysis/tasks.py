# coding: utf-8

import law.util
from zhh import get_raw_files
import law
import luigi

# import our "framework" tasks
from analysis.framework import HTCondorWorkflow
from phc.tasks import ShellTask, BaseTask
from zhh import plot_preselection_pass
from phc import export_figures

import numpy as np
import uproot as ur
import os.path as osp

class Preselection(ShellTask, HTCondorWorkflow, law.LocalWorkflow):
    debug = luigi.BoolParameter(default=False)
    nmax = luigi.IntParameter(default=100)
    
    def create_branch_map(self) -> dict[int, str]:
        arr = get_raw_files()
        
        # for debugging: only first two entries
        if self.nmax > 0:
            arr = arr[:min(self.nmax, len(arr))]
        
        if self.debug:
            arr = arr[:2]
        
        res = { k: v for k, v in zip(list(range(len(arr))), arr) }
        
        return res

    def output(self):
        return self.target_collection([
            self.local_target(f'{self.branch}/zhh_FinalStates.root'),
            self.local_target(f'{self.branch}/zhh_Preselection.root')
        ])

    def build_command(self, fallback_level):
        output_root = osp.dirname(str(self.output()[0].path))
        
        cmd =  f'source /afs/desy.de/user/b/bliewert/public/MarlinWorkdirs/ZHH/setup.sh'
        cmd += f' && mkdir -p output'
        cmd += f' && Marlin $REPO_ROOT/scripts/newZHHllbbbb.xml {"" if (self.debug == True) else "--global.MaxRecordNumber=0 "}--global.LCIOInputFiles={self.branch_map[self.branch]}'
        cmd += f' && mkdir -p {output_root} && mv output/* {output_root}'

        return cmd


class CreatePlots(BaseTask):
    """
    This task requires the Preselection workflow and extracts the created data to create plots.
    """

    def requires(self):
        return Preselection.req(self)

    def output(self):
        # output a plain text file
        return self.local_target("plots.pdf")

    def run(self):
        # since we require the workflow and not the branch tasks (see above), self.input() points
        # to the output of the workflow, which contains the output of its branches in a target
        # collection, stored - of course - in "collection"
        inputs = self.input()['collection'].targets
        
        files = []
        for input_coll in inputs.values():
            for key, input in input_coll._iter_flat():
                path = input[0].path
                print(path)
                if path.endswith('Preselection.root'):
                    files.append(path)

        vecs = []
        for f in files:
            d = ur.open(f)['eventTree']
            vecs.append(np.array(d['preselsPassedVec'].array()))
            
        vecs = np.concatenate(vecs)
            
        figs = plot_preselection_pass(vecs)
        
        # Create intermediate directories and save plots
        self.output().parent.touch()
        export_figures(self.output().path, figs)
        
        # Status message
        self.publish_message(f'exported {len(figs)} plots to {self.output().path}')
