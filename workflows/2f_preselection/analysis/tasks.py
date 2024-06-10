# coding: utf-8

import law.util
from zhh import get_raw_files
import law
from law.util import flatten
import luigi

# import our "framework" tasks
from analysis.framework import HTCondorWorkflow
from zhh import plot_preselection_pass
from phc import export_figures, ShellTask, BaseTask, ForcibleTask

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
        return [
            self.local_target(f'{self.branch}_PreSelection.root'),
            self.local_target(f'{self.branch}_FinalStates.root'),
            self.local_target(f'{self.branch}_FinalStateMeta.json')
        ]

    def build_command(self, fallback_level):
        temp_files = self.output()
        
        cmd =  f'source $REPO_ROOT/setup.sh'
        cmd += f' && Marlin $REPO_ROOT/scripts/newZHHllbbbb.xml {"" if (self.debug == True) else "--global.MaxRecordNumber=0 "}--global.LCIOInputFiles={self.branch_map[self.branch]} --constant.OutputDirectory=.' # --constant.OutputBaseName={self.branch}_zhh'
        cmd += f' && mv zhh_PreSelection.root {temp_files[0].path}'
        cmd += f' && mv zhh_FinalStates.root {temp_files[1].path}'
        cmd += f' && mv zhh_FinalStateMeta.json {temp_files[1].path}'

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
        # Get targets of dependendies and get the file paths of relevant files 
        inputs = self.input()['collection'].targets
        
        files = []
        for input in flatten(inputs):
            if input.path.endswith('PreSelection.root'):
                files.append(input.path)
                
        # Extract columns using uproot
        vecs = []
        for f in files:
            d = ur.open(f)['eventTree']
            vecs.append(np.array(d['preselsPassedVec'].array()))
            
        vecs = np.concatenate(vecs)
        vecs[vecs < 0] = 0 # setting invalid entries to 0
        
        # Create the plots and save them to
        figs = plot_preselection_pass(vecs)
        
        self.output().parent.touch() # Create intermediate directories and save plots    
        export_figures(self.output().path, figs)
        
        # Status message
        self.publish_message(f'exported {len(figs)} plots to {self.output().path}')


