# coding: utf-8

import law.util
from zhh import get_raw_files
import law
from law.util import flatten
import luigi

# import our "framework" tasks
from analysis.framework import HTCondorWorkflow
from zhh import plot_preselection_pass, is_readable
from phc import export_figures, ShellTask, BaseTask, ForcibleTask

import numpy as np
import uproot as ur
import os.path as osp

class Preselection(ShellTask, HTCondorWorkflow, law.LocalWorkflow):
    debug = luigi.BoolParameter(default=False)
    
    def requires(self):
        return Preselection.req(self)
    
    def create_branch_map(self) -> dict[int, str]:
        arr = get_raw_files(debug=self.debug)
        arr = list(filter(is_readable, arr))
        
        res = { k: v for k, v in zip(list(range(len(arr))), arr) }
        
        return res

    def output(self):
        return [
            self.local_target(f'{self.branch}_PreSelection_llHH.root'),
            self.local_target(f'{self.branch}_PreSelection_vvHH.root'),
            self.local_target(f'{self.branch}_PreSelection_qqHH.root'),
            self.local_target(f'{self.branch}_FinalStates.root'),
            self.local_target(f'{self.branch}_FinalStateMeta.json'),
            self.local_target(f'{self.branch}_Source.txt')
        ]

    def build_command(self, fallback_level):
        temp_files = self.output()
        src_file = str(self.branch_map[self.branch])
        
        # Check if sample belongs to new or old MC production to change MCParticleCollectionName
        mcp_col_name = 'MCParticlesSkimmed' if '/hh/' in src_file else 'MCParticle'
        
        cmd =  f'source $REPO_ROOT/setup.sh'
        cmd += f' && echo "Starting Marlin at $(date)"'
        
        src_txt_target = self.local_path(str(self.branch) + "_Source.txt")
        root_files = ['PreSelection_llHH.root', 'PreSelection_vvHH.root', 'PreSelection_qqHH.root', 'FinalStates.root']
        for suffix in ['FinalStateMeta.json'] + root_files:
            target_path = self.local_path(str(self.branch) + "_" + suffix)
            cmd += f' && (if [[ ! -f {src_txt_target} && -f {target_path} ]]; then rm {target_path}; fi)'
        
        cmd += f' && ( Marlin $REPO_ROOT/scripts/ZHH_v2.xml --global.MaxRecordNumber={"20" if self.debug else "0"} --global.LCIOInputFiles={src_file} --constant.OutputDirectory=. --constant.MCParticleCollectionName={mcp_col_name} || true )'
        cmd += f' && echo "Finished Marlin at $(date)"'
        
        # is_root_readable
        for suffix in root_files:
            cmd += f' && is_root_readable ./zhh_{suffix}'
        
        cmd += f' && mv zhh_PreSelection_llHH.root {temp_files[0].path}'
        cmd += f' && mv zhh_PreSelection_vvHH.root {temp_files[1].path}'
        cmd += f' && mv zhh_PreSelection_qqHH.root {temp_files[2].path}'
        cmd += f' && mv zhh_FinalStates.root {temp_files[3].path}'
        cmd += f' && mv zhh_FinalStateMeta.json {temp_files[4].path}'
        cmd += f' && echo "{self.branch_map[self.branch]}" >> {temp_files[5].path}'

        return cmd

class CreateIndex(BaseTask):
    """
    This task creates two indeces: An index of available SLCIO sample files with information about the file location, number of events, physics process and polarization + an index containing all encountered physics processes for each polarization and their cross section-section values 
    """
    
    def output(self):
        return [
            self.local_target('processes.json'),
            self.local_target('samples.json')
        ]
    

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


