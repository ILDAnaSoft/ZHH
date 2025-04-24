import os.path as osp
import law
from analysis.framework import zhh_configs
from typing import Callable, cast
from .tasks_abstract import FastSimSGVExternalReadJob
from .utils.types import SGVOptions
from law.util import flatten

class FastSimSGV(FastSimSGVExternalReadJob):
    branch_data: tuple[str, SGVOptions]
    
    initial_run_with_higher_requirements = False
    
    def workflow_requires(self):
        reqs = super().workflow_requires()
        zhh_configs.get(str(self.tag)).sgv_requires(self, reqs) # call to inject dynamic workflow requirements
        
        return reqs
    
    @law.dynamic_workflow_condition
    def workflow_condition(self):
        return all(cast(law.FileSystemTarget, elem).exists() for elem in flatten(self.input()))
        
    @workflow_condition.create_branch_map
    def create_branch_map(self):
        config = zhh_configs.get(str(self.tag))
        assert(isinstance(config.sgv_inputs, Callable))
        
        input_files, input_options = config.sgv_inputs(self)
        assert(len(input_files) == len(input_options))
        
        return { k: [file, options] for (k, file, options) in zip(
            list(range(len(input_files))),
            input_files,
            input_options
        )}
    
    @workflow_condition.output
    def output(self):
        # output filename = input filename but extension changed to 'slcio'; necessary for stdhep input
        return self.local_target(f'{osp.splitext(osp.basename(self.branch_data[0]))[0]}.slcio')