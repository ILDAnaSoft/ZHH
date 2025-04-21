from analysis.framework import zhh_configs
from typing import Callable
import os.path as osp
from .tasks_abstract import FastSimSGVExternalReadJob
from .utils.types import SGVOptions

class FastSimSGV(FastSimSGVExternalReadJob):
    branch_data: tuple[str, SGVOptions]
    
    def requires(self):
        return zhh_configs.get(str(self.tag)).sgv_requires(self)
          
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
        
    def output(self):
        # output filename = input filename but extension changed to 'slcio'; necessary for stdhep input
        return self.local_target(f'{osp.splitext(osp.basename(self.branch_data[0]))[0]}.slcio')