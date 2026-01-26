from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor
from zhh import find_by

def get_signal_categories(signal_classes:list[int], mva:dict)->list[int]:
    from zhh import EventCategories
    indices = []

    for label, name in mva['classes']:
        if getattr(EventCategories, name) in signal_classes:
            indices.append(label)

    return indices

class SklearnMulticlassInferenceAction(CutflowProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, use:str, from_file:str, split:int,
                 step:int|None=None, from_file_property:str='clf', **kwargs):
        """_summary_

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            use (str): _description_
            from_file (str): pickle file in which the classifier is stored
            from_file_property (str, optional): key at which to find the classifier. Defaults to 'clf'.
        """
        assert('mvas' in steer)

        super().__init__(cp, steer)

        mva_spec = find_by(steer['mvas'], 'name', use, is_dict=True)
        
        self._file = from_file        
        self._clf_property = from_file_property
        self._step = step
        self._split = split
        self._output_label = mva_spec['label_name']

        self._signal_classes = get_signal_categories(steer['signal_classes'], mva_spec)
        self._classes = mva_spec['classes']
        self._features = mva_spec['features']
    
    def run(self):
        import pickle
        import numpy as np

        try:
            with open(self._file, 'rb') as bf:
                trial = pickle.load(bf)
                xgbclf = trial[self._clf_property]
        except Exception as e:
            raise Exception(f'Could not find a classifier in pickle file <{self._file}> at location <{self._clf_property}>')

        sources = self._cp.getSources()
        features = self._features

        for source in sources:
            print(f'Evaluating MVA for source <{source}>')
            
            store = source.getStore()
            store.resetView()
            mask = self._cp.getFinalEventMaskByName(source.getName(), step=self._step) & (store['split'] == self._split) # select test data

            inputs = np.zeros((mask.sum(), len(features)))
            bdtg_sig = np.zeros(len(store))
            
            if mask.sum():
                for j, feature in enumerate(features):
                    inputs[:, j] = store[feature][mask]
                
                mva = xgbclf
                probas = mva.predict_proba(inputs)
                bdtg_sig[mask] = probas[:, self._signal_classes].sum(axis=1)
            
            store[self._output_label] = bdtg_sig

    def complete(self) -> bool:
        for source in self._cp.getSources():
            store = source.getStore()
            
            if not self._output_label in store:
                return False
            
        return True