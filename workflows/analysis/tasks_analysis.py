from analysis.tasks_abstract import MarlinJob
from typing import Union, Optional
from law.util import flatten
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
        ('OutputDirectory', '.')
    ]
    
    check_output_root_ttrees = [
        ('zhh_PreSelection_llHH.root', 'eventTree'),
        ('zhh_PreSelection_vvHH.root', 'eventTree'),
        ('zhh_PreSelection_qqHH.root', 'eventTree'),
        ('zhh_FinalStates.root', 'eventTree')
    ]
    
    check_output_files_exist = [
        'zhh_FinalStateMeta.json'
    ]
    
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
        
        return all(elem.exists() for elem in flatten(self.input()))
    
    # The decorator @workflow_condition.create_branch_map is required
    # for all workflows which require a branch map conditioned on the
    # output of a previous task (in this case, RawIndex)
    @workflow_condition.create_branch_map
    def create_branch_map(self) -> Union[
        dict[int, str],
        dict[int, tuple[str, int, int]]
        ]:
        samples = np.load(self.input()['raw_index'][1].path)
        
        if not self.debug:
            # The calculated chunking is used
            scs = np.load(self.input()['preselection_chunks'][0].path)
            branch_map = { k: v for k, v in zip(scs['branch'].tolist(), zip(scs['location'], scs['chunk_start'], scs['chunk_size'], ['MCParticlesSkimmed']*len(scs))) }
        else:
            # A debug run. The default settings
            # from the steering file are used
            selection = samples[np.lexsort((samples['location'], samples['proc_pol']))]
            
            # Average over three runs for each proc_pol
            # run to get a more accurate runtime estimate
            arr = []
            for proc_pol in np.unique(selection['proc_pol']):
                for location in selection['location'][selection['proc_pol'] == proc_pol][:3]:
                    arr.append(location)
                
            branch_map = { k: v for k, v in zip(list(range(len(arr))), arr) }
        
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
    
