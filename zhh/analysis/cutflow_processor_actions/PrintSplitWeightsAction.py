from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor
import numpy as np

class PrintSplitWeightsAction(CutflowProcessorAction):
    transforms_data = False

    def __init__(self, cp:CutflowProcessor, steer:dict):
        super().__init__(cp, steer)
        self._complete = False
    
    def run(self):
        print('PrintSplitWeights: ')

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