from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor

class CreateCutflowPlotsAction(FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, file:str, **kwargs):
        super().__init__(cp, steer)
        self._file = file
        self._kwargs = kwargs
    
    def run(self):
        self._cp.cutflowPlots(display=False, file=self._file, **self._kwargs)
    
    def output(self):
        return self.localTarget(self._file)