
from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor
from .mva_tools import get_signal_categories
from tqdm.auto import tqdm
from math import ceil

class AttachMatrixElementsAction(CutflowProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, output_name:str, data_source:str,
                 step:int|None=None, progress:bool=True, **kwargs):
        """_summary_

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            mva (str): _description_
            clf_prop (str, optional): key at which to find the classifier. Defaults to 'clf'.
        """
        assert('mvas' in steer)

        super().__init__(cp, steer)

        assert(data_source.upper() in ['KINFIT'])

        self._output_name = output_name
        self._data_source = data_source.upper()
        self._step = step
        self._progress = progress
    
    def run(self):
        import numpy as np
        from zhh import fill_mem_momenta, load_jet_matching_kinfit
        from zhh.mem.Physsim.build.PhyssimWrapper import calc_me_zhh, calc_me_zzh

        sources = self._cp.getSources()

        for source in (pbar := tqdm(sources, disable=not self._progress)):
            pbar.set_description(f'Loading jet matching for source <{source}>...')

            store = source.getStore()
            store.resetView()

            if self._data_source == 'KINFIT':
                jet_matching = load_jet_matching_kinfit(store)
            
            CHUNK_SIZE = 100000

            output_all = np.zeros(len(store), dtype=np.float32)

            for i in (pbar := tqdm(range(0, ceil(len(store) / CHUNK_SIZE)))):
                size = min(CHUNK_SIZE, abs(len(store) - CHUNK_SIZE * i))
                output = np.zeros(size, dtype=np.float32)
                
                mask = np.zeros(len(store), dtype=bool)
                mask[i*CHUNK_SIZE:i*CHUNK_SIZE + size] = True

                p_zhh, p_zzh = fill_mem_momenta(store, jmk=jet_matching[mask] , mask=mask)
                
                p_invalid = (p_zhh[:, 0] == 0) & (p_zhh[:, 1] == 0)
                output[p_invalid] = np.nan

                p_zhh = p_zhh[~p_invalid, :]
                p_zzh = p_zzh[~p_invalid, :]

                me_zhh = calc_me_zhh(-.8, .3, 5, p_zhh)
                me_zzh = calc_me_zzh(-.8, .3, 5, 5, p_zzh)

                output[~p_invalid] = me_zhh/me_zzh
                output_all[i*CHUNK_SIZE:i*CHUNK_SIZE + size] = output
            
            # assign and save to HDF5
            store[self._output_name] = output
            store.itemsSnapshot(items=[self._output_name])

    def complete(self) -> bool:
        for source in self._cp.getSources():
            store = source.getStore()
            
            if not self._output_name in store:
                return False
            
        return True
    
    def reset(self):
        for source in self._cp.getSources():
            store = source.getStore()
            
            if store.hasProperty(self._output_name):
                store.removeProperty(self._output_name)