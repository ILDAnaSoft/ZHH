from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor
from ..Cuts import Cut
import os

class ApplyCutsAction(CutflowProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, step:int, cuts:str,
                 weight_column:str, split:int|None, cache:str|None=None, **kwargs):
        """_summary_

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            step (int): Incrementing index of cut group
            cuts (str): Name of cut group
            cache (str|None): Path to pickle file for caching of the preselection
            weight_column (str): Column to extract the weights from
            split (int|None): Which split to use (i.e. training/testing etc.)
                              If None, no data split, i.e. all data will be used.
                              If weight_column is the default value (weight), this
                              is set to None.
        """

        super().__init__(cp, steer)

        self._step = step
        self._cuts = steer['cuts'][cuts]
        self._weight_column = weight_column
        self._split = None if weight_column == 'weight' else split
        self._cache = os.path.expandvars(cache) if isinstance(cache, str) else None
    
    def fetchCuts(self):
        from zhh import cutflow_parse_cuts
        return cutflow_parse_cuts(self._cuts, mvas=self._cp._mvas)

    def run(self):
        self._cp.process(step=self._step, cuts=self.fetchCuts(),
                         weight_prop=self._weight_column, split=self._split,
                         cache=self._cache)

    def complete(self)->bool:
        # preload from cache
        if self._step in self._cp._calc_dicts:
            return True
        elif self._step == 0 and self._cache is not None and os.path.isfile(self._cache):
            cuts_steer = self.fetchCuts()
            self._cp.process(step=self._step, cuts=cuts_steer, cache=self._cache)

            return Cut.hash_cuts(cuts_steer) == self._cp._cuts_hash
        else:
            return False
    
    def reset(self):
        # only preselection/step=0 is cached in CutflowProcessor.process()
        if self._cache is not None:
            if self._step == 0 and os.path.isfile(self._cache):
                os.remove(self._cache)
        
        if self._step in self._cp._masks:
            for step in self._cp._masks.keys():
                if step >= self._step:
                    del self._cp._masks[step]
                    del self._cp._calc_dicts[step]
                    del self._cp._max_before[step]