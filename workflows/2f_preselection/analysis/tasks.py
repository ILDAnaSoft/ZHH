# coding: utf-8

import law.util
from zhh import get_raw_files
import law
from law.util import flatten
import luigi

# import our "framework" tasks
from analysis.framework import HTCondorWorkflow
from phc.tasks import ShellTask, BaseTask
from zhh import plot_preselection_pass
from phc import export_figures, ForcibleTask

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
            self.local_target(f'{self.branch}/zhh_FinalStates.root'),
            self.local_target(f'{self.branch}/zhh_PreSelection.root')
        ]

    def build_command(self, fallback_level):
        output_root = osp.dirname(str(self.output()[0].path))
        
        cmd =  f'source $REPO_ROOT/setup.sh'
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


class SetupPackages(ShellTask, ForcibleTask, law.LocalWorkflow):
    packages = ['AddNeutralPFOCovMat', 'CheatedMCOverlayRemoval', 'LeptonPairing', 'HdecayMode', 'PreSelection', 'FinalStateRecorder']
    
    def output(self):
        return self.local_target('build_complete')
    
    def create_branch_map(self) -> dict[int, str]:
        return { k: self.packages[k] for k in range(len(self.packages)) }
    
    def complete(self):
        package = self.branch_data
        return (not self.force) and (osp.isfile(f'$REPO_ROOT/source/{package}/lib/lib{package}.so'))
    
    def build_command(self, fallback_level):
        package = self.branch_data
        
        cmd = f"""(source $REPO_ROOT/setup.sh &&{f' rm -rf "$REPO_ROOT/source/{package}/build" && rm -rf "$REPO_ROOT/source/{package}/lib" &&' if self.force else ''} mkdir -p "$REPO_ROOT/source/{package}/build" && cd "$REPO_ROOT/source/{package}/build" && ( if [ ! -f "$REPO_ROOT/source/{package}/lib/lib{package}.so" ]; then cmake .. && make install; fi ) && mkdir -p $(dirname "{self.output().path}") && touch "{self.output().path}" ) || exit 11"""

        return cmd