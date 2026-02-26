from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor
from .mva_tools import get_signal_categories
from tqdm.auto import tqdm

class SklearnMulticlassInferenceAction(CutflowProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, mva:str, split:int,
                 step:int|None=None, clf_prop:str='clf', progress:bool=True, **kwargs):
        """_summary_

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            mva (str): _description_
            clf_prop (str, optional): key at which to find the classifier. Defaults to 'clf'.
        """
        assert('mvas' in steer)

        super().__init__(cp, steer)

        from zhh import find_by

        mva_spec = find_by(steer['mvas'], 'name', mva, is_dict=True)
        
        self._mva_file = mva_spec['mva_file']
        self._mva_name = mva

        self._clf_prop = clf_prop
        self._step = step
        self._split = split
        self._output_label = mva_spec['label_name']

        self._signal_categories = get_signal_categories(steer['signal_categories'], mva_spec['classes'])        
        self._features = mva_spec['features']
        self._progress = progress
    
    def run(self):
        import pickle
        import numpy as np

        try:
            with open(self._mva_file, 'rb') as bf:
                trial = pickle.load(bf)
                mva = trial[self._clf_prop]
        except Exception as e:
            raise Exception(f'Could not find a classifier in pickle file <{self._mva_file}> at location <{self._clf_prop}>')

        sources = self._cp.getSources()
        features = self._features

        for source in (pbar := tqdm(sources, disable=not self._progress)):
            pbar.set_description(f'Loading MVA feature data for source <{source}>...')
            
            store = source.getStore()
            store.resetView()
            mask = self._cp.getFinalEventMaskByName(source.getName(), step=self._step) & (store['split'] == self._split) # select test data

            inputs = np.zeros((mask.sum(), len(features)))
            bdtg_sig = np.zeros(len(store))
            
            if mask.sum():
                for j, feature in enumerate(features):
                    pbar.set_description(f'Loading feature {j+1}/{len(features)} ({feature}) for MVA <{self._mva_name}> of source <{source}>')
                    inputs[:, j] = store[feature][mask]
                    
                pbar.set_description(f'Evaluating MVA for source <{source}>...')
                
                probas = mva.predict_proba(inputs)
                bdtg_sig[mask] = probas[:, self._signal_categories].sum(axis=1)
            
            # assign and save to HDF5
            store[self._output_label] = bdtg_sig
            store.itemsSnapshot(items=[self._output_label])

    def complete(self) -> bool:
        for source in self._cp.getSources():
            store = source.getStore()
            
            if not self._output_label in store:
                return False
            
        return True
    
    def reset(self):
        for source in self._cp.getSources():
            store = source.getStore()
            
            if store.hasProperty(self._output_label):
                store.removeProperty(self._output_label)       