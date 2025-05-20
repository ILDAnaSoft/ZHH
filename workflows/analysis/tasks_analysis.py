from analysis.tasks_abstract import MarlinJob, MarlinBranchValue
from typing import Union, Optional, cast
from collections.abc import Callable
from law.util import flatten
from analysis.framework import zhh_configs
import numpy as np
import os.path as osp
import law

class AnalysisAbstract(MarlinJob):
    debug = True
    
    # controls how many files are processed in debug mode (e.g. AnalysisRuntime). if None, all files are processed
    debug_n_files_to_process:int|None = 3
    
    constants = [
        ('ILDConfigDir', '$ILD_CONFIG_DIR'), # read from environment variable
        ('ZHH_REPO_ROOT', '$REPO_ROOT'),
        ('OutputDirectory', '.')
    ]
    
    check_output_root_ttrees = [
        ('zhh_AIDA.root', 'EventObservablesLL'),
        #('zhh_AIDA.root', 'KinFitLLZHH'),
        #('zhh_AIDA.root', 'KinFitLLZZH'),
        ('zhh_AIDA.root', 'FinalStates')
    ]
    
    check_output_files_exist = [
        'zhh_FinalStateMeta.json'
    ]
    
    # Attach MCParticleCollectionName and constants/globals for Marlin
    def pre_run_command(self, **kwargs):
        config = zhh_configs.get(str(self.tag))
        
        if 'MarlinJob' in config.task_kwargs:
            for prop, value in config.task_kwargs['MarlinJob'].items():
                setattr(self, prop, value)
 
        mcp_col_name:str = self.get_steering_parameters()['mcp_col_name']
        
        self.constants.append(('MCParticleCollectionName', mcp_col_name))
        
        marlin_constants = config.marlin_constants(self.branch, self.branch_data) if isinstance(config.marlin_constants, Callable) else config.marlin_constants
        for key, value in marlin_constants.items():
            self.constants.append((key, str(value)))
        
        for key, value in config.marlin_globals.items():
            self.globals.append((key, str(value)))
    
    def workflow_requires(self):
        from analysis.tasks import RawIndex, CreateAnalysisChunks
        
        reqs = super().workflow_requires()
        reqs['raw_index'] = RawIndex.req(self)
        
        # In debug mode (runtime analysis), the CreateAnalysisChunks task is not required
        if not self.debug:
            reqs['preselection_chunks'] = CreateAnalysisChunks.req(self)
        
        return reqs
    
    @law.dynamic_workflow_condition
    def workflow_condition(self):
        # declare that the branch map can be built only if the workflow requirement exists
        # the decorator will trigger a run of workflow_requires beforehand
        # because of the decorator, self.input() will refer to the outputs of tasks defined in workflow_requires()
        
        return all(cast(law.FileSystemTarget, elem).exists() for elem in flatten(self.input()))
    
    # The decorator @workflow_condition.create_branch_map is required
    # for all workflows which require a branch map conditioned on the
    # output of a previous task (in this case, RawIndex)
    @workflow_condition.create_branch_map
    def create_branch_map(self) -> dict[int, MarlinBranchValue]:
        branch_map:dict[int, MarlinBranchValue] = {}
        
        config = zhh_configs.get(str(self.tag))
        if 'MarlinJob' in config.task_kwargs:
            for prop, value in config.task_kwargs['MarlinJob'].items():
                setattr(self, prop, value)
        
        samples = np.load(self.input()['raw_index'][1].path)
        
        if not self.debug:
            # The calculated chunking is used
            scs = np.load(self.input()['preselection_chunks'][0].path)
            
            for branch in scs['branch'].tolist():
                branch_map[branch] = (
                    scs['location'][branch],
                    scs['n_chunk_in_sample'][branch],
                    scs['n_chunks_in_sample'][branch],
                    scs['chunk_start'][branch],
                    scs['chunk_size'][branch],
                    samples['mcp_col_name'][samples['location'] == scs['location'][branch]][0]
                )
                
            #branch_map = { k: v for k, v in zip(
            #    scs['branch'].tolist(),
            #    zip(scs['location'],
            #        scs['chunk_start'],
            #        scs['chunk_size'],
            #        samples['mcp_col_name'][scs['sid']])) }
        else:
            
            # A debug run. The default settings
            # from the steering file are used
            selection = samples[np.lexsort((samples['location'], samples['proc_pol']))]
            
            i = 0
            
            for proc_pol in np.unique(selection['proc_pol']):
                items = selection[selection['proc_pol'] == proc_pol]
                if self.debug_n_files_to_process is not None and self.debug_n_files_to_process > 0:
                    items = items[:self.debug_n_files_to_process]
                
                for entry in items:
                    branch_map[i] = (
                        entry['location'],
                        0,
                        1,
                        -1,
                        -1,
                        entry['mcp_col_name'] 
                    )
                    i += 1
        
        return branch_map

    @workflow_condition.output
    def output(self):
        output_name = self.output_name()
        
        return [
            self.local_directory_target(output_name),
            self.local_target(f'{output_name}.root')
        ]
        
        #return self.local_directory_target(self.branch)

class AnalysisRuntime(AnalysisAbstract):
    """Generates a runtime analysis for each proc_pol combination, essentially by running in debug mode

    Args:
        AnalysisAbstract (_type_): _description_
    """
    debug = True

class AnalysisFinal(AnalysisAbstract):
    """Does the full analysis for every channel and proc_pol combination

    Args:
        AnalysisAbstract (_type_): _description_
    """
    debug = False
    
