from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
import numpy as np

class PlotMVAFeaturesAction(FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, mva:str, file:str, **kwargs):
        assert('mvas' in steer)

        super().__init__(cp, steer)

        from zhh import find_by

        mva_spec = find_by(steer['mvas'], 'name', mva, is_dict=True)

        self._data_file = mva_spec['data_file']
        self._plot_file = file
        self._kwargs = kwargs
    
    def run(self):
        for source in self._cp.getSources():
            print('Source =', source.getName())
            store = source.getStore()

            split_tag = store['split']
            weights_new = store['weights_split']
            
            print(f'wt pre-split = {store["weight"].sum()}')
            
            for tag in np.unique(split_tag):
                print(f'wt-tot for split={tag}: {weights_new[split_tag == tag].sum()}')

        self._complete = True
    
    def complete(self):
        return self._complete