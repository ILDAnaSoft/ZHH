from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor
import os

class ApplyCutsAction(CutflowProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, step:int, cuts:str, **kwargs):
        """_summary_

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            step (int): Incrementing index of cut group
            cuts (str): Name of cut group
        """

        super().__init__(cp, steer)

        self._step = step
        self._cuts = steer['cuts'][cuts]
        self._cache = 'cutflow_presel.pickle'
    
    def fetchCuts(self):
        from zhh import cutflow_parse_cuts
        return cutflow_parse_cuts(self._cuts, mvas=self._cp._mvas)

    def run(self):
        self._cp.process(step=self._step, cuts=self.fetchCuts())

    def complete(self)->bool:
        # preload from cache
        if self._step in self._cp._calc_dicts:
            return True
        elif self._step == 0 and os.path.isfile(self._cache):
            self._cp.process(step=self._step, cuts=self.fetchCuts(), cache=self._cache)
            return True
        else:
            return False
    
    def reset(self):
        # only preselection/step=0 is cached in CutflowProcessor.process()
        if self._step == 0 and os.path.isfile(self._cache):
            os.remove(self._cache)
        
        if self._step in self._cp._masks:
            for step in self._cp._masks.keys():
                if step >= self._step:
                    del self._cp._masks[step]
                    del self._cp._calc_dicts[step]
                    del self._cp._max_before[step]