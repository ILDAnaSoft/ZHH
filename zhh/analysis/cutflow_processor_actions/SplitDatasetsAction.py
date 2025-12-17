from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor

class SplitDatasetsAction(CutflowProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, fractions:list[float],
                 split_column:str='split', wt_split_column:str='weights_split', **kwargs):
        """Randomly splits all datasets associated with a CutflowProcessor into len(fractions)+1 categories
        by a label column named by split_column, where the fraction of each label is given by fractions.
        Re-calculated weights are calculated such that the sum of weights within dataset matches the sum
        before the split and assigned to the column weights_split.

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            fractions (list[float]): list of n fractions for each of the n+1 categories
                                    fraction of category n+1 is calculated as 1-(...)
            split_column (str, optional): _description_. Defaults to 'split'.
            wt_split_column (str, optional): _description_. Defaults to 'weights_split'.
        """
        super().__init__(cp, steer)
        
        self._fractions = fractions
        self._split_column = split_column
        self._wt_split_column = wt_split_column

    def run(self):
        from zhh import apply_split, mod_weights_from_split

        for source in self._cp.getSources():
            store = source.getStore()
            
            apply_split(*self._fractions, source=source, name=self._split_column)
            store[self._wt_split_column] = mod_weights_from_split(source)
            
            store.itemsSnapshot(overwrite=True, items=[self._split_column, self._wt_split_column])
    
    def reset(self):
        for source in self._cp.getSources():
            store = source.getStore()
            [store.removeProperty(prop) for prop in [self._split_column, self._wt_split_column]]

    def complete(self):
        for source in self._cp.getSources():
            store = source.getStore()
            if not self._split_column in store or not self._wt_split_column in store:
                return False

        return True