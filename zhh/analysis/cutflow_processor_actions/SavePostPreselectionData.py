from ..CutflowProcessorAction import CutflowProcessor, FileBasedProcessorAction
from tqdm.auto import tqdm
from ..DataSource import DataSource
import os
import numpy as np

class SavePostPreselectionData(FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict,
                 cache:str='$hypothesis_cutflow_presel.pickle',
                 step:int=0, progress:bool=True, overwrite:bool=True, **kwargs):
        """_summary_

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            mva (str): _description_
            clf_prop (str, optional): key at which to find the classifier. Defaults to 'clf'.
            overwrite (bool, optional): whether or not the output MVA variable should over#
                                        write existing values (if any)
        """

        super().__init__(cp, steer)

        self._step = step
 
        self._cache = os.path.expandvars(cache) if isinstance(cache, str) else None
        self._progress = progress
        self._overwrite = overwrite
    
    def run(self):
        sources = self._cp.getSources()

        for source in (pbar := tqdm(sources, disable=not self._progress)):
            pbar.set_description(f'Loading MVA feature data for source <{source}>...')
            
            store = source.getStore()
            store.resetView()
            probas = self.calculateMVAValuesOfSource(source)
            
            # assign and save to HDF5
            store[self._output_label] = probas[:, self._signal_categories].sum(axis=1)
            store.itemsSnapshot(items=[self._output_label], overwrite=self._overwrite)

            for cat in self._signal_categories:
                signal_label_name = f'{self._output_label}#{cat}'
                
                store[signal_label_name] = probas[cat].sum(axis=1)
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
        assert(self._mva is not None)

        self.loadMVAFromFile()

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
        
        return probas

    def complete(self) -> bool:
        for source in self._cp.getSources():
            store = source.getStore()
            
            if not self._output_label in store:
                return False
            
        return True
    
    def reset(self):
        if os.path.isfile(self._cache):
            os.remove(self._cache)