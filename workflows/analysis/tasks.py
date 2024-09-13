import luigi, law, json, os, uuid
from law.util import flatten
from math import ceil
from law import LocalFileTarget

from analysis.framework import HTCondorWorkflow
from zhh import get_raw_files, presel_stack, plot_preselection_pass, is_readable, ProcessIndex, \
    get_adjusted_time_per_event, get_runtime_analysis, get_sample_chunk_splits, get_process_normalization, \
    get_preselection_passes, get_chunks_factual, get_final_state_counts
from phc import export_figures, ShellTask, BaseTask, ForcibleTask

from typing import Optional, Union, Annotated, List
import numpy as np
import uproot as ur
import os.path as osp

class CreateRawIndex(BaseTask):
    """
    This task creates two indeces: An index of available SLCIO sample files with information about the file location, number of events, physics process and polarization + an index containing all encountered physics processes for each polarization and their cross section-section values 
    """
    index: Optional[ProcessIndex] = None
    
    def output(self):
        return [
            self.local_target('processes.npy'),
            self.local_target('samples.npy')
        ]
    
    def run(self):
        temp_files: List[LocalFileTarget] = self.output()
        
        temp_files[0].parent.touch()
        self.index = index = ProcessIndex(temp_files[0].path, temp_files[1].path, get_raw_files())
        self.index.load()
        
        self.publish_message(f'Loaded {len(index.samples)} samples and {len(index.processes)} processes')

class CreatePreselectionChunks(BaseTask):
    ratio: Annotated[float, luigi.FloatParameter()] = 1.
    jobtime: Annotated[int, luigi.IntParameter()] = 7200
    
    def requires(self):
        return [
            CreateRawIndex.req(self),
            PreselectionRuntime.req(self)
        ]

    def output(self):
        return [
            self.local_target('chunks.npy'),
            self.local_target('runtime_analysis.npy'),
            self.local_target('atpe.npy'),
            self.local_target('process_normalization.npy'),
            self.local_target('arguments.json')
        ]
    
    def run(self):
        SAMPLE_INDEX = self.input()[0][1].path
        DATA_ROOT = osp.dirname(self.input()[1]['collection'][0].path)
        
        processes = np.load(self.input()[0][0].path)
        samples = np.load(SAMPLE_INDEX)
        
        runtime_analysis = get_runtime_analysis(DATA_ROOT)
        pn = get_process_normalization(processes, samples, RATIO_BY_EXPECT=self.ratio)
        atpe = get_adjusted_time_per_event(runtime_analysis)
        
        with open(osp.expandvars(f'$REPO_ROOT/workflows/analysis/custom_statistics.json'), 'r') as f:
            custom_statistics = json.load(f)

        chunk_splits = get_sample_chunk_splits(samples, process_normalization=pn,
                    adjusted_time_per_event=atpe, MAXIMUM_TIME_PER_JOB=self.jobtime,
                    custom_statistics=custom_statistics)
        
        self.output()[0].parent.touch()
        
        np.save(self.output()[0].path, chunk_splits)
        np.save(self.output()[1].path, runtime_analysis)
        np.save(self.output()[2].path, atpe)
        np.save(self.output()[3].path, pn)
        
        self.output()[4].dump({'ratio': float(self.ratio), 'jobtime': int(self.jobtime)})
        
        self.publish_message(f'Compiled preselection chunks')

class UpdatePreselectionChunks(BaseTask):
    """Updates the chunk definitions by only appending new chunks
    for greater statistics. Useful only if the reconstruction has not
    changed.

    Args:
        BaseTask (_type_): _description_
    """
    
    def requires(self):
        return [
            CreateRawIndex.req(self),
            CreatePreselectionChunks.req(self)
        ]

    def output(self):
        return self.local_target('chunks.npy')
    
    def run(self):
        chunks_in = self.input()[1][0]
        samples = np.load(self.input()[0][1].path)
        atpe = np.load(self.input()[1][2].path)
        pn = np.load(self.input()[1][3].path)
        arguments = self.input()[1][4].load()
        
        chunks_in_path = chunks_in.path
        existing_chunks = np.load(chunks_in_path)
        
        with open(osp.expandvars(f'$REPO_ROOT/workflows/analysis/custom_statistics.json'), 'r') as f:
            custom_statistics = json.load(f)
        
        new_chunks = get_sample_chunk_splits(samples, atpe, pn, MAXIMUM_TIME_PER_JOB=arguments['jobtime'], \
                    custom_statistics=custom_statistics, existing_chunks=existing_chunks)
        
        chunks_in.remove()
        np.save(chunks_in_path, new_chunks)
        
        self.output().parent.touch()
        np.save(self.output().path, new_chunks)
        
        self.publish_message(f'Updated chunks. Added {len(new_chunks)-len(existing_chunks)} branches.')

class PreselectionAbstract(ShellTask, HTCondorWorkflow, law.LocalWorkflow):
    debug = True
    
    def workflow_requires(self):
        reqs = super().workflow_requires()
        reqs['raw_index'] = CreateRawIndex.req(self)
        
        if not self.debug:
            reqs['preselection_chunks'] = CreatePreselectionChunks.req(self)
        
        return reqs
    
    @law.dynamic_workflow_condition
    def workflow_condition(self):
        # declare that the branch map can be built only if the workflow requirement exists
        # the decorator will trigger a run of workflow_requires beforehand
        if len(self.input()) > 0:
            # here: self.input() refers to the outputs of tasks defined in workflow_requires()
            return all(elem.exists() for elem in flatten(self.input()))
            #return self.input()["raw_index"][0].exists() and self.input()["raw_index"][1].exists()
        else:
            return True
    
    @workflow_condition.create_branch_map
    def create_branch_map(self) -> Union[
        dict[int, str],
        dict[int, tuple[str, int, int]]
        ]:
        samples = np.load(self.input()['raw_index'][1].path)
        
        if 'preselection_chunks' in self.input():
            scs = np.load(self.input()['preselection_chunks'][0].path)
            res = { k: v for k, v in zip(scs['branch'].tolist(), zip(scs['location'], scs['chunk_start'], scs['chunk_size']))}
        else:             
            selection = samples[np.lexsort((samples['location'], samples['proc_pol']))]
            
            # Average over three runs for each proc_pol run to get a more accurate estimate
            arr = []
            for proc_pol in np.unique(selection['proc_pol']):
                for location in selection['location'][selection['proc_pol'] == proc_pol][:3]:
                    arr.append(location)
                
            res = { k: v for k, v in zip(list(range(len(arr))), arr) }
        
        return res

    @workflow_condition.output
    def output(self):
        return self.local_directory_target(self.branch)

    def get_target_and_temp(self, branch):
        return (
            f'{self.htcondor_output_directory().path}/{branch}',
            f'{self.htcondor_output_directory().path}/{branch}-{str(uuid.uuid4())}'
        )

    def build_command(self, fallback_level):
        branch = self.branch
        src = self.branch_map[branch]
        
        if isinstance(src, tuple):
            src_file = src[0]
            n_events_skip = src[1]
            n_events_max = src[2]
        else:
            src_file = str(src)
            n_events_skip = 0
            n_events_max = 50 if self.debug else 0
        
        target, temp = self.get_target_and_temp(branch)
        os.makedirs(osp.dirname(target), exist_ok=True)
        
        # Check if sample belongs to new or old MC production to change MCParticleCollectionName
        mcp_col_name = 'MCParticlesSkimmed' if '/hh/' in src_file else 'MCParticle'
        
        cmd =  f'source $REPO_ROOT/setup.sh'
        cmd += f' && echo "Starting Marlin at $(date)"'
        cmd += f' && mkdir -p "{temp}" && cd "{temp}"'

        # Marlin fix for SkipNEvents=0
        if n_events_skip == 0:
            n_events_max = n_events_max + 1
        
        cmd += f' && ( Marlin $REPO_ROOT/scripts/ZHH_v2.xml --global.MaxRecordNumber={str(n_events_max)} --global.LCIOInputFiles={src_file} --global.SkipNEvents={str(n_events_skip)} --constant.OutputDirectory=. --constant.MCParticleCollectionName={mcp_col_name} || true )'
        cmd += f' && echo "Finished Marlin at $(date)"'
        
        # is_root_readable (necessary because CheatedMCOverlayRemoval crashes Marlin before it can properly exit, see above)
        for suffix in ['_PreSelection_llHH.root', '_PreSelection_vvHH.root', '_PreSelection_qqHH.root', '_FinalStates.root']:
            cmd += f' && is_root_readable ./zhh{suffix}'
        
        cmd += f' && echo "{self.branch_map[self.branch]}" >> Source.txt'
        cmd += f' && cd .. && mv "{temp}" "{target}"'

        return cmd
    
class PreselectionRuntime(PreselectionAbstract):
    """Generates a runtime analysis for each proc_pol combination, essentially by running in debug mode

    Args:
        PreselectionAbstract (_type_): _description_
    """
    debug = True

class PreselectionFinal(PreselectionAbstract):
    debug = False # luigi.BoolParameter(default=False)

class PreselectionSummary(BaseTask, HTCondorWorkflow):
    branchesperjob: Annotated[int, luigi.IntParameter()] = 256
    
    preselection_chunks: Optional[str] = None
    processes_index: Optional[str] = None
    
    def workflow_requires(self):
        reqs = super().workflow_requires()
        reqs['preselection_final'] = PreselectionFinal.req(self)
        reqs['preselection_chunks'] = CreatePreselectionChunks.req(self)
        reqs['raw_index'] = CreateRawIndex.req(self)
        
        return reqs
    
    @law.dynamic_workflow_condition
    def workflow_condition(self):
        if len(self.input()) > 0:
            # here: self.input() refers to the outputs of tasks defined in workflow_requires()
            return all(elem.exists() for elem in flatten(self.input()))
        else:
            return True
    
    @workflow_condition.create_branch_map
    def create_branch_map(self):
        n_branches_in = len(self.input()['preselection_final']['collection'])
        n_branches = ceil(n_branches_in / self.branchesperjob)
        DATA_ROOT = osp.dirname(self.input()['preselection_final']['collection'][0][0].path)

        branch_key = np.arange(n_branches_in)
        branch_val = np.split(branch_key, self.branchesperjob*np.arange(1, n_branches))
        
        preselection_chunks = self.input()['preselection_chunks'][0].path
        processes_index = self.input()['raw_index'][0].path

        return dict(
            zip(branch_key.tolist(), zip(
                [DATA_ROOT] * n_branches,
                branch_val,
                [preselection_chunks] * n_branches,
                [processes_index] * n_branches
                )))
    
    def output(self):
        return [
            self.local_target(f'{self.branch}_Presel.npy'),
            self.local_target(f'{self.branch}_FinalStates.npy')
        ]

    def run(self):
        src = self.branch_map[self.branch]
        DATA_ROOT, branches, preselection_chunks, processes_index = src
        
        chunks = np.load(preselection_chunks)
        chunks_factual = get_chunks_factual(DATA_ROOT, chunks)
        
        processes = np.load(processes_index)
        
        output = self.output()
        output[0].parent.touch()
        
        presel_result = presel_stack(DATA_ROOT, processes, chunks_factual, branches, kinematics=True)
        np.save(self.output()[0].path, presel_result)
        
        final_states = get_final_state_counts(DATA_ROOT, branches, chunks_factual)
        np.save(self.output()[1].path, final_states)
        
        self.publish_message(f'Processed {len(branches)} branches')
    

class CreatePlots(BaseTask):
    """
    This task requires the Preselection workflow and extracts the created data to create plots.
    """

    def requires(self):
        return PreselectionFinal.req(self)

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


