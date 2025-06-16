import numpy as np
import os, luigi, law, asyncio
from law.util import query_choice, colored
import os.path as osp
from glob import glob
from .utils import ForcibleTask, ShellTask, BaseTask
from analysis.framework import zhh_configs

class ClearLCIOFiles(BaseTask):
    task_completed = False
    
    def requires(self):
        from analysis.tasks_analysis import AnalysisFinal
        return [ AnalysisFinal.req(self) ]
    
    def complete(self):
        # check if the output file exists
        return self.task_completed
    
    def run(self):
        source_dirn = osp.dirname(self.input()[0]['collection'][0][0].path)
        files = glob(f'{source_dirn}/**/*.slcio', recursive=True)
        tot_size_in_bytes = np.sum([os.path.getsize(f) for f in files])
        
        #confirmation = input(f'Are you sure you want to delete all ({len(files)}) *.slcio files under {source_dirn} (y/n)? ')
        confirmation = query_choice(f'Are you sure you want to delete all <{len(files)}> *.slcio files under {source_dirn}> with total size {(tot_size_in_bytes / 1024 / 1024):.1f} MB? ', ['y', 'n'], default='n')
        
        if confirmation == 'y':
            for f in files:
                os.remove(f)
                
            self.task_completed = True 
            self.publish_message(colored(f'Successfully deleted {len(files)} files!', color='green')) 
        else:
            self.publish_message(colored(f'Aborted', color='red')) 