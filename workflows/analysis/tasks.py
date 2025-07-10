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
from .tasks_abstract import AbstractIndex, AbstractCreateChunks

class RawIndex(AbstractIndex):
    pass

class AnalysisIndex(AbstractIndex):
    pass

class CreateRecoChunks(AbstractCreateChunks):
    def requires(self):
        from analysis.tasks_marlin import RecoRuntime
        return [ RawIndex.req(self), RecoRuntime.req(self) ]

class CreateAnalysisChunks(AbstractCreateChunks):
    def requires(self):
        from analysis.tasks_marlin import AnalysisRuntime
        return [ AnalysisIndex.req(self), AnalysisRuntime.req(self) ]
        

class AnalysisCombine(ShellTask):    
    def requires(self):
        from analysis.tasks_marlin import AnalysisFinal
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
        from analysis.tasks_marlin import AnalysisFinal
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