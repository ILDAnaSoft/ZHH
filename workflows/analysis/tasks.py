import luigi, law, json, os, uuid, subprocess
from law.util import flatten
from math import ceil
from law import LocalFileTarget

from analysis.framework import HTCondorWorkflow, zhh_configs
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
    def requires(self):
        from analysis.configurations import zhh_configs
        return zhh_configs.get(str(self.tag)).raw_index_requires(self)

class AnalysisIndex(AbstractIndex):
    def requires(self):
        from analysis.configurations import zhh_configs
        return zhh_configs.get(str(self.tag)).analysis_index_requires(self)
    
    def slcio_files(self):
        reco_final_target_collection = self.input()[0]['collection']
        
        # reco_final_target_collection[i][0] is directory, [i][1] is file
        reco_slcio_files = [reco_final_target_collection[i][1].path for i in range(len(reco_final_target_collection))]
        
        return reco_slcio_files

class CreateRecoChunks(AbstractCreateChunks):
    jobtime:int = cast(int, luigi.IntParameter(description='Maximum runtime of each job. Uses DESY NAF defaults for the vanilla queue.',
                                               default=7200))
    
    T0_MARLIN = 16
    
    def requires(self):
        from analysis.tasks_marlin import RecoRuntime
        return [ RawIndex.req(self), RecoRuntime.req(self) ]

class CreateAnalysisChunks(AbstractCreateChunks):
    jobtime:int = cast(int, luigi.IntParameter(description='Maximum runtime of each job. Uses DESY NAF defaults for the vanilla queue.',
                                               default=3600))
    
    T0_MARLIN = 4
    
    def requires(self):
        from analysis.tasks_marlin import AnalysisRuntime
        return [ AnalysisIndex.req(self), AnalysisRuntime.req(self), CreateRecoChunks.req(self) ]
        

class AnalysisCombine(ShellTask):    
    def requires(self):
        from analysis.tasks_marlin import AnalysisFinal
        return [ AnalysisFinal.req(self) ]

    def output(self):
        return self.local_target('Merged.root')

    def build_command(self, **kwargs):
        from analysis.configurations import zhh_configs
        ttrees = zhh_configs.get(str(self.tag)).analysis_combine_ttrrees
    
        output_dirn = osp.dirname(cast(str, self.output().path))
        source_dirn = osp.dirname(self.input()[0]['collection'][0][0].path)
        
        return f"""source $REPO_ROOT/setup.sh
zhhvenv
python $REPO_ROOT/zhh/cli/merge_root_files.py "{output_dirn}" "{",".join(ttrees)}]" --dirs="{source_dirn}"
echo Success""".replace('\n', '  &&  ')


import analysis.configurations