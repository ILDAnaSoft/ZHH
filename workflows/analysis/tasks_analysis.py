from analysis.tasks_abstract import MarlinJob
from typing import Union, Optional, cast
from law.util import flatten
from analysis.framework import zhh_configs
import numpy as np
import law

class AnalysisAbstract(MarlinJob):
    debug = True
    
    constants = [
        ('ILDConfigDir', '$ILD_CONFIG_DIR'), # read from environment variable
        ('ZHH_REPO_ROOT', '$REPO_ROOT'),
        ('Runllbbbb', 'True'),
        ('Runvvbbbb', 'True'),
        ('Runqqbbbb', 'True'),
        ('RunKinfit', 'True'),
        ('RunTruthRecoComparison', 'False'),
        ('OutputDirectory', '.')
    ]
    
    check_output_root_ttrees = [
        ('zhh_PreSelection_llHH.root', 'PreSelection'),
        ('zhh_PreSelection_vvHH.root', 'PreSelection'),
        ('zhh_PreSelection_qqHH.root', 'PreSelection'),
        #('zhh_TruthRecoComparison.root', 'TruthRecoComparison'),
        ('zhh_FinalStates.root', 'FinalStates')
    ]
    
    check_output_files_exist = [
        'zhh_FinalStateMeta.json'
    ]
    
    # Attach MCParticleCollectionName and constants/globals for Marlin
    def pre_run_command(self, **kwargs):
        config = zhh_configs.get(str(self.tag))
 
        mcp_col_name:str = self.get_steering_parameters()['mcp_col_name']
        
        self.constants.append(('MCParticleCollectionName', mcp_col_name))
        
        for key, value in config.marlin_constants.items():
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
    def create_branch_map(self) -> Union[
        dict[int, dict],
        dict[int, tuple[str, int, int, str]]
        ]:
        samples = np.load(self.input()['raw_index'][1].path)
        
        if not self.debug:
            # The calculated chunking is used
            scs = np.load(self.input()['preselection_chunks'][0].path)
            branch_map = { k: v for k, v in zip(
                scs['branch'].tolist(),
                zip(scs['location'],
                    scs['chunk_start'],
                    scs['chunk_size'],
                    samples['mcp_col_name'][scs['sid']])) }
        else:
            # A debug run. The default settings
            # from the steering file are used
            selection = samples[np.lexsort((samples['location'], samples['proc_pol']))]
            branch_map = {}
            i = 0
            
            for proc_pol in np.unique(selection['proc_pol']):
                for entry in selection[selection['proc_pol'] == proc_pol][:3]:
                    branch_map[i] = {
                        'location': entry['location'],
                        'mcp_col_name': entry['mcp_col_name'] 
                    }
                    i += 1
        
        temp = {  }
        temp[0] = branch_map[0]
        
        return branch_map

    @workflow_condition.output
    def output(self):
        return self.local_directory_target(self.branch)

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
    
