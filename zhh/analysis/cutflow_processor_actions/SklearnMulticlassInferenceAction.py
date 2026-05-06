from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor
from .mva_tools import get_signal_categories
from tqdm.auto import tqdm
from ..DataSource import DataSource
import numpy as np

class SklearnMulticlassInferenceAction(CutflowProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, mva:str, split:int,
                 step:int|None=None, clf_prop:str='clf', progress:bool=True, overwrite:bool=True, **kwargs):
        """_summary_

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            mva (str): _description_
            clf_prop (str, optional): key at which to find the classifier. Defaults to 'clf'.
            overwrite (bool, optional): whether or not the output MVA variable should over#
                                        write existing values (if any)
        """
        assert('mvas' in steer)

        super().__init__(cp, steer)

        from zhh import find_by

        mva_spec = find_by(steer['mvas'], 'name', mva, is_dict=True)
        
        self._mva_file = mva_spec['mva_file']
        self._mva_name = mva
        self._mva = None

        self._clf_prop = clf_prop
        self._step = step
        self._split = split
        self._output_label = mva_spec['label_name']

        self._signal_categories = get_signal_categories(steer['signal_categories'], mva_spec['classes'])
        self._class_names = [ item[1] for item in  mva_spec['classes'] ]
        self._features = mva_spec['features']
        self._progress = progress
        self._overwrite = overwrite
    
    def run(self):
        sources = self._cp.getSources()

        for source in (pbar := tqdm(sources, disable=not self._progress)):
            pbar.set_description(f'Loading MVA feature data for source <{source}>...')
            
            store = source.getStore()
            store.resetView()
            mask, probas = self.calculateMVAValuesOfSource(source)
            
            # assign and save total signal score to HDF5
            score = np.zeros(len(mask))
            score[mask] = probas[:, self._signal_categories].sum(axis=1)

            store[self._output_label] = score
            store.itemsSnapshot(items=[self._output_label], overwrite=self._overwrite)

            # assign and save individual scores to HDF5
            for i, category in enumerate(self._class_names):                
                signal_label_name = f'{self._output_label}#{category}'
                
                score = np.zeros(len(mask))
                score[mask] = probas[:, i]

                store[signal_label_name] = mask
                store.itemsSnapshot(items=[signal_label_name], overwrite=self._overwrite)

    def loadMVAFromFile(self):
        if self._mva is None:
            import pickle

            try:
                with open(self._mva_file, 'rb') as bf:
                    trial = pickle.load(bf)
                    self._mva = trial[self._clf_prop]
            except Exception as e:
                raise Exception(f'Could not find a classifier in pickle file <{self._mva_file}> at location <{self._clf_prop}>')

    def calculateMVAValuesOfSource(self, source:DataSource, pbar:tqdm|None=None):
        self.loadMVAFromFile()

        if self._mva is None or not hasattr(self._mva, 'predict_proba'):
            raise Exception('MVA does not support predict_proba() method')

        features = self._features
        store = source.getStore()
        store.resetView()

        mask = self._cp.getFinalEventMaskByName(source.getName(), step=self._step) & (store['split'] == self._split) # select test data

        inputs = np.zeros((mask.sum(), len(features)))
        
        if mask.sum():
            for j, feature in enumerate(features):
                if pbar is not None:
                    pbar.set_description(f'Loading feature {j+1}/{len(features)} ({feature}) for MVA <{self._mva_name}> of source <{source}>')
                
                inputs[:, j] = store[feature][mask]
            
            if pbar is not None:
                pbar.set_description(f'Evaluating MVA for source <{source}>...')
            
            probas = self._mva.predict_proba(inputs)
        
        return (mask, probas)

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
            
            for cat in self._signal_categories:
                signal_label_name = f'{self._output_label}#{cat}'

                if store.hasProperty(signal_label_name):
                    store.removeProperty(signal_label_name)