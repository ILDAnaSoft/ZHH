from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor

class CreateCutflowPlotsAction(FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, file:str, step_start:int=0, step_end:int=0, **kwargs):
        super().__init__(cp, steer)
        self._file = file
        self._step_start = step_start
        self._step_end = step_end
        self._kwargs = kwargs
    
    def run(self):        
        self._cp.cutflowPlots(display=False, file=self._file,
                              step_start=self._step_start, step_end=self._step_end, **self._kwargs)
    
    def output(self):
        return self.localTarget(self._file)