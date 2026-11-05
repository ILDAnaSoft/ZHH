from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
import os.path as osp

class CreateCutflowPlotsAction(FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, file:str, step_start:int=0, step_end:int=0,
                 steps:list[int]|None=None, **kwargs):
        """Creates the cutflow plots (distribution of the cut variable before each cut and event counts per category).

        Args:
            file (str): output file (PDF)
            step_start (int, optional): first cut group. Ignored if steps is given. Defaults to 0.
            step_end (int, optional): last cut group (inclusive). Ignored if steps is given. Defaults to 0.
            steps (list[int], optional): explicit list of cut groups, in this order, e.g. [0, 2] for the
                preselection followed by cut group 2 (which was set up with step_in=0), skipping group 1.
                Defaults to None.
        """
        super().__init__(cp, steer)
        self._file = osp.expandvars(file)
        self._step_start = step_start
        self._step_end = step_end
        self._steps = steps
        self._kwargs = kwargs
    
    def run(self):        
        self._cp.cutflowPlots(display=False, file=self._file,
                              step_start=self._step_start, step_end=self._step_end, steps=self._steps, **self._kwargs)
    
    def output(self):
        return self.localTarget(self._file)