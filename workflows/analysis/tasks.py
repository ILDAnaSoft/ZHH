import luigi, law, json, os, uuid
from law.util import flatten
from math import ceil
from law import LocalFileTarget

from analysis.framework import HTCondorWorkflow, zhh_configs, AnalysisConfiguration

from zhh import get_raw_files, analysis_stack, ProcessIndex, \
    get_adjusted_time_per_event, get_runtime_analysis, get_sample_chunk_splits, get_process_normalization, \
    get_chunks_factual, BaseTask

from typing import Optional, Union, Annotated
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
        config = zhh_configs.get(str(self.tag))
        if config.index_requires is not None:
            return config.index_requires(self)
        else:
            return []
    
    def slcio_files(self) -> list[str]:
        config = zhh_configs.get(str(self.tag))
        if callable(config.slcio_files):
            files = config.slcio_files(self)
        else:
            files = config.slcio_files
            
        files.sort()
        return files
    
    def output(self):
        return [
            self.local_target('processes.npy'),
            self.local_target('samples.npy')
        ]
    
    def run(self):
        temp_files: list[law.LocalFileTarget] = self.output()
        
        temp_files[0].parent.touch()
        self.index = index = ProcessIndex(temp_files[0].path, temp_files[1].path, self.slcio_files())
        self.index.load()
        
        self.publish_message(f'Loaded {len(index.samples)} samples and {len(index.processes)} processes')

class CreateAnalysisChunks(BaseTask):
    jobtime: Annotated[int, luigi.IntParameter()] = 7200
    
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
        DATA_ROOT = osp.dirname(self.input()[1]['collection'][0].path)
        
        processes = np.load(self.input()[0][0].path)
        samples = np.load(SAMPLE_INDEX)
        
        runtime_analysis = get_runtime_analysis(DATA_ROOT)
        pn = get_process_normalization(processes, samples, RATIO_BY_TOTAL=config.statistics)
        atpe = get_adjusted_time_per_event(runtime_analysis)

        chunk_splits = get_sample_chunk_splits(samples, process_normalization=pn,
                    adjusted_time_per_event=atpe, MAXIMUM_TIME_PER_JOB=self.jobtime,
                    custom_statistics=config.custom_statistics)
        
        self.output()[0].parent.touch()
        
        np.save(self.output()[0].path, chunk_splits)
        np.save(self.output()[1].path, runtime_analysis)
        np.save(self.output()[2].path, atpe)
        np.save(self.output()[3].path, pn)
        
        #self.output()[4].dump({'ratio': float(self.ratio), 'jobtime': int(self.jobtime)})
        
        self.publish_message(f'Compiled analysis chunks')
    

class AnalysisSummary(BaseTask, HTCondorWorkflow):
    dtype: Annotated[str, luigi.Parameter()] = 'numpy'
    branchesperjob: Annotated[int, luigi.IntParameter()] = 256
    
    analysis_chunks: Optional[str] = None
    processes_index: Optional[str] = None
    
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
            return all(elem.exists() for elem in flatten(self.input()))
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
    
    def output(self):
        if not (str(self.dtype).lower() in ['root', 'numpy']):
            raise ValueError(f'Unknown output dtype <{self.dtype}>')
        
        return self.local_target(f'{self.branch}_Presel.{"npy" if self.dtype == "numpy" else "root"}')

    def run(self):
        from zhh import numpy2root
        
        src = self.branch_map[self.branch]
        DATA_ROOT, branches, analysis_chunks, processes_index = src
        
        chunks = np.load(analysis_chunks)
        chunks_factual = get_chunks_factual(DATA_ROOT, chunks)
        
        processes = np.load(processes_index)
        
        output = self.output()
        output.parent.touch()
        
        presel_result = analysis_stack(DATA_ROOT, processes, chunks_factual, branches,
                                     kinematics=True, b_tagging=True, final_states=True)
        
        if self.dtype == 'numpy':
            np.save(output.path, presel_result)
        else:
            numpy2root(presel_result, output.path, 'summary')
        
        self.publish_message(f'Processed {len(branches)} branches')

import analysis.configurations