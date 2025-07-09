import luigi, law, json, os, uuid, subprocess
from law.util import flatten
from math import ceil
from law import LocalFileTarget

from analysis.framework import HTCondorWorkflow, zhh_configs, aa_configs, AggregateAnalysisConfig
from .utils import ShellTask, BaseTask, RealTimeLoggedTask
from zhh import get_raw_files, analysis_stack, ProcessIndex, \
    get_adjusted_time_per_event, get_runtime_analysis, get_sample_chunk_splits, get_process_normalization, \
    get_chunks_factual

from typing import Optional, cast
from glob import glob
import numpy as np
import os.path as osp

class RawIndex(BaseTask):
    """This task creates two indeces:
    1. samples.npy: An index of available SLCIO sample files with information about the file location, number of events, physics process and polarization
    2. processes.npy: An index containing all encountered physics processes for each polarization and their cross section-section values 
    """
    index: Optional[ProcessIndex] = None
    
    def requires(self):
        return zhh_configs.get(str(self.tag)).index_requires(self)
    
    def slcio_files(self) -> list[str]:
        config = zhh_configs.get(str(self.tag))
        if callable(config.slcio_files):
            files = config.slcio_files(self)
        elif config.slcio_files is not None:
            files = config.slcio_files
        else:
            raise Exception(f'Invalid slcio_files in config <{self.tag}>')
            
        files.sort()
        return files
    
    def output(self):
        return [
            self.local_target('processes.npy'),
            self.local_target('samples.npy')
        ]
    
    def run(self):
        temp_files: list[law.LocalFileTarget] = self.output()
        BaseTask.touch_parent(temp_files[0])

        self.index = index = ProcessIndex(str(temp_files[0].path), str(temp_files[1].path), self.slcio_files())
        self.index.load()
        
        # For compatability, also save as CSV
        np.savetxt(osp.join(osp.dirname(str(temp_files[0].path)), 'samples.csv'), index.samples, delimiter=',', fmt='%s')
        np.savetxt(osp.join(osp.dirname(str(temp_files[1].path)), 'processes.csv'), index.processes, delimiter=',', fmt='%s')
        
        self.publish_message(f'Loaded {len(index.samples)} samples and {len(index.processes)} processes')

class CreateAnalysisChunks(BaseTask):
    jobtime = cast(int, luigi.IntParameter(default=7200))
    
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
    
    def requires(self):
        from analysis.tasks_analysis import AnalysisRuntime
        
        return [
            RawIndex.req(self),
            AnalysisRuntime.req(self)
        ]

    def output(self):
        return [
            self.local_target('chunks.npy'),
            self.local_target('runtime_analysis.npy'),
            self.local_target('atpe.npy'),
            self.local_target('process_normalization.npy'),
            #self.local_target('arguments.json')
        ]
    
    def run(self):
        config = zhh_configs.get(str(self.tag))
        
        SAMPLE_INDEX = self.input()[0][1].path
        DATA_ROOT = osp.dirname(self.input()[1]['collection'][0][0].path)
        
        processes = np.load(self.input()[0][0].path)
        samples = np.load(SAMPLE_INDEX)
        
        runtime_analysis = get_runtime_analysis(DATA_ROOT)
        pn = get_process_normalization(processes, samples, RATIO_BY_TOTAL=config.statistics)
        atpe = get_adjusted_time_per_event(runtime_analysis)

        chunk_splits = get_sample_chunk_splits(samples, process_normalization=pn,
                    adjusted_time_per_event=atpe, MAXIMUM_TIME_PER_JOB=cast(int, self.jobtime),
                    custom_statistics=config.custom_statistics)
        
        BaseTask.touch_parent(self.output()[0])
        
        np.save(str(self.output()[0].path), chunk_splits)
        np.save(str(self.output()[1].path), runtime_analysis)
        np.save(str(self.output()[2].path), atpe)
        np.save(str(self.output()[3].path), pn)
        
        # For compatability, also save the final results as CSV
        np.savetxt(osp.join(osp.dirname(str(self.output()[0].path)), 'chunk_splits.csv'), chunk_splits, delimiter=',', fmt='%s')
        np.savetxt(osp.join(osp.dirname(str(self.output()[1].path)), 'runtime_analysis.csv'), runtime_analysis, delimiter=',', fmt='%s')
        np.savetxt(osp.join(osp.dirname(str(self.output()[2].path)), 'time_per_event.csv'), atpe, delimiter=',', fmt='%s')
        np.savetxt(osp.join(osp.dirname(str(self.output()[3].path)), 'normalization.csv'), pn, delimiter=',', fmt='%s')
        
        self.publish_message(f'Compiled analysis with {len(chunk_splits)} chunks!')
    

class AnalysisSummary(BaseTask, HTCondorWorkflow):
    dtype = cast(str, luigi.Parameter(default='numpy'))
    branchesperjob = cast(int, luigi.IntParameter(default=256))
    
    analysis_chunks: Optional[str] = None
    processes_index: Optional[str] = None
    
    branch_data: tuple[str, list[str], str, str]
    
    def workflow_requires(self):
        from analysis.tasks_analysis import AnalysisFinal
        
        reqs = super().workflow_requires()
        reqs['analysis_final'] = AnalysisFinal.req(self)
        reqs['analysis_chunks'] = CreateAnalysisChunks.req(self)
        reqs['raw_index'] = RawIndex.req(self)
        
        return reqs
    
    @law.dynamic_workflow_condition
    def workflow_condition(self):
        if len(self.input()) > 0:
            # here: self.input() refers to the outputs of tasks defined in workflow_requires()
            return all(elem.exists() for elem in cast(list[LocalFileTarget], flatten(self.input())))
        else:
            return True
    
    @workflow_condition.create_branch_map
    def create_branch_map(self):
        n_branches_in = len(self.input()['analysis_final']['collection'])
        n_branches = ceil(n_branches_in / self.branchesperjob)
        DATA_ROOT = osp.dirname(self.input()['analysis_final']['collection'][0].path)

        branch_key = np.arange(n_branches_in)
        branch_val = np.split(branch_key, self.branchesperjob*np.arange(1, n_branches))
        
        analysis_chunks = self.input()['analysis_chunks'][0].path
        processes_index = self.input()['raw_index'][0].path

        return dict(
            zip(branch_key.tolist(), zip(
                [DATA_ROOT] * n_branches,
                branch_val,
                [analysis_chunks] * n_branches,
                [processes_index] * n_branches
                )))
    
    @workflow_condition.output
    def output(self):
        dtype = self.dtype.lower()
        if not (dtype in ['root', 'numpy']):
            raise ValueError(f'Unknown output dtype <{dtype}>')
        
        self.postfix = f'-{dtype}'
        
        return self.local_target(f'{self.branch}_Presel.{"npy" if dtype == "numpy" else "root"}')

    def run(self):
        from zhh import numpy2root
        
        src = self.branch_data
        DATA_ROOT, branches, analysis_chunks, processes_index = src
        
        chunks:np.ndarray = np.load(analysis_chunks)
        chunks_factual = get_chunks_factual(DATA_ROOT, chunks)
        
        processes:np.ndarray = np.load(processes_index)
        
        output = self.output()
        BaseTask.touch_parent(output)
        
        presel_result = analysis_stack(DATA_ROOT, processes, chunks_factual, branches,
                                     kinematics=True, b_tagging=True, final_states=True)
        
        if self.dtype == 'numpy':
            np.save(str(output.path), presel_result)
        else:
            numpy2root(presel_result, str(output.path), 'summary')
            
        if self.branch == 0:
            np.save(osp.join(osp.dirname(str(output.path)), 'chunks_factual.npy'), chunks_factual)
            np.savetxt(osp.join(osp.dirname(str(output.path)), 'chunks_factual.csv'), chunks_factual, delimiter=',', fmt='%s')
        
        self.publish_message(f'Processed {len(branches)} branches')

class AnalysisCombine(ShellTask):    
    def requires(self):
        from analysis.tasks_analysis import AnalysisFinal
        return [ AnalysisFinal.req(self) ]

    def output(self):
        return self.local_target('Merged.root')

    def build_command(self, **kwargs):
        output_dirn = osp.dirname(cast(str, self.output().path))
        source_dirn = osp.dirname(self.input()[0]['collection'][0][0].path)
        ttrees = ['FinalStates', 'EventObservablesLL', 'KinFit_solveNu', 'KinFit_ZHH', 'KinFit_ZZH']
        
        return f"""source $REPO_ROOT/setup.sh
zhhvenv
python $REPO_ROOT/zhh/cli/merge_root_files.py "{output_dirn}" "{",".join(ttrees)}]" --dirs="{source_dirn}"
echo Success""".replace('\n', '  &&  ')


class AnalysisCombineAA(BaseTask, HTCondorWorkflow):
    dtype = cast(str, luigi.Parameter(default='numpy'))
    
    def workflow_requires(self):
        from analysis.tasks_analysis import AnalysisFinal
        config:AggregateAnalysisConfig = aa_configs.get(str(self.tag))
        
        reqs = super().workflow_requires()
        tag_prev = self.tag
        
        for sub_tag in config.sub_tags:
            self.tag = sub_tag
            analysis_final_instance = AnalysisFinal.req(self)
            
            reqs['analysis_final_' + sub_tag.replace('-', '_')] = analysis_final_instance
        
        self.tag = tag_prev
        
        return reqs
    
    @law.dynamic_workflow_condition
    def workflow_condition(self):
        if len(self.input()) > 0:
            # here: self.input() refers to the outputs of tasks defined in workflow_requires()
            return all(elem.exists() for elem in cast(list[LocalFileTarget], flatten(self.input())))
        else:
            return True
    
    @workflow_condition.create_branch_map
    def create_branch_map(self):
        print(self.input())
        
        branch_map = {}
        raise NotImplementedError('Not implemented yet')

        return branch_map
    
    @workflow_condition.output
    def output(self):
        raise NotImplementedError('Not implemented yet')

    def run(self):
        raise NotImplementedError('Not implemented yet')

import analysis.configurations