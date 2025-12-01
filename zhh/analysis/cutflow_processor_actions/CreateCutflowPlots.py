from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor
import os.path as osp

class CreateCutflowPlots(CutflowProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, file:str, **kwargs):
        super().__init__(cp)
        self._file = file
        self._kwargs = kwargs
    
    def run(self):
        self._cp.cutflowPlots(display=False, file=self._file, **self._kwargs)
    
    def done(self):
        return osp.isfile(self._file)