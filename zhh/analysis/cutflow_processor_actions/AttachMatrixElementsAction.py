
from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor
from .mva_tools import get_signal_categories
from tqdm.auto import tqdm
import numpy as np

class AttachMatrixElementsAction(CutflowProcessorAction):
    """Attaches the ratio of the ZHH and ZZH matrix elements, ME(ZHH)/ME(ZZH).
    Only the lepton channel is supported
    """

    # mapping of PDG of the Z decay fermion -> LCME decay mode code; see
    # EventObservablesBase::m_pdg_to_lcme_mode
    PDG_TO_LCME_MODE = { 5: 11, 11: 4, 13: 5, 15: 6, }

    def __init__(self, cp:CutflowProcessor, steer:dict, output_name:str, data_source:str,
                 step:int|None=None, progress:bool=True, chunk_size:int=100000,
                 z_lepton_pdg:int=13, propagator_type:int=2, higgs_width:float=10.,
                 invalid_output_value:float=np.nan, **kwargs):
        """Attaches the ratio of the ZHH and ZZH matrix elements, ME(ZHH)/ME(ZZH), as column
        <output_name> to the DataStore of each source. Matrix elements are evaluated with the
        jet matching found by the ZHH/ZZH KinFit for the respective hypothesis, and only for events
        passing selectEvents(); for all other events, <output_name> is set to <invalid_output_value>.

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): steering dict; beam_polarization is used as polarization of the matrix elements
            output_name (str): name of the column to write
            data_source (str): source of the jet matching. only 'kinfit' is implemented
            chunk_size (int): number of events per call to the matrix element code
            z_lepton_pdg (int): PDG ID (absolute) of the leptons from the Z decay; 13 for the muon channel
            propagator_type (int): Higgs propagator of the matrix elements. 0 = disabled, 1 = Breit-Wigner
                                   with the SM width (needle-narrow), 2 = Gaussian with detector resolution
                                   (default; see scratch/development/matrix_elements/mem_study/mem_lib.py)
            higgs_width (float): width of the Higgs propagator in GeV (for propagator_type=2 the detector
                                 resolution; 10 GeV was calibrated on oracle-matched ZHH Higgs candidates)
            invalid_output_value (float): value set when no matrix element could be calculated. defaults to
                                          np.nan
        """
        assert('mvas' in steer)

        super().__init__(cp, steer)

        assert(data_source.upper() in ['KINFIT'])
        assert(z_lepton_pdg in self.PDG_TO_LCME_MODE)

        self._output_name = output_name
        self._data_source = data_source.upper()
        self._step = step
        self._progress = progress
        self._chunk_size = chunk_size
        self._z_lepton_mode = self.PDG_TO_LCME_MODE[z_lepton_pdg]
        self._propagator_type = propagator_type
        self._higgs_width = higgs_width
        self._pol_e, self._pol_p = steer.get('beam_polarization', [-.8, .3])
        self._invalid_output_value = invalid_output_value

    def loadJetMatching(self, store)->tuple[np.ndarray, np.ndarray]:
        """Returns the (N,4) jet matchings to use for the ZHH and ZZH matrix element, respectively.
        Entries of a row that is not a valid matching must be -1. Override/extend for other jet
        matching methods."""
        from zhh.mem import load_jet_matching_kinfit

        if self._data_source == 'KINFIT':
            return load_jet_matching_kinfit(store, 'zhh'), load_jet_matching_kinfit(store, 'zzh')

        raise Exception(f'Unknown data_source <{self._data_source}>')

    def selectEvents(self, store, jm_zhh:np.ndarray, jm_zzh:np.ndarray)->np.ndarray:
        """Returns a boolean mask over the events in the store, selecting those for which the matrix
        elements are evaluated. As the matrix element code does not validate its input, this must at
        least exclude events without a valid jet matching. Currently, that is the only criterion."""
        from zhh.mem import valid_jet_matching_mask

        return valid_jet_matching_mask(jm_zhh) & valid_jet_matching_mask(jm_zzh)

    def run(self):
        from zhh.mem import fill_mem_momenta
        from zhh.mem.Physsim.build.PhyssimWrapper import calc_me_zhh_jm, calc_me_zzh_jm

        sources = self._cp.getSources()

        for source in (pbar := tqdm(sources, disable=not self._progress)):
            pbar.set_description(f'Loading jet matching for source <{source}>...')

            store = source.getStore()
            store.resetView()

            jm_zhh, jm_zzh = self.loadJetMatching(store)
            selected = self.selectEvents(store, jm_zhh, jm_zzh)

            # events outside of the selection set to invalid_output_value
            # float64: ratio can exceed float32 range
            output_all = np.full(len(store), self._invalid_output_value, dtype=np.float64)
            idx = np.flatnonzero(selected)

            if len(idx):
                pbar.set_description(f'Loading momenta of {len(idx)}/{len(store)} events for source <{source}>...')

                momenta = fill_mem_momenta(store, mask=selected)
                jm_zhh = np.ascontiguousarray(jm_zhh[idx])
                jm_zzh = np.ascontiguousarray(jm_zzh[idx])

                for start in tqdm(range(0, len(idx), self._chunk_size), disable=not self._progress,
                                  desc=f'Calculating matrix elements for source <{source}>'):
                    chunk = slice(start, start + self._chunk_size)

                    me_zhh = calc_me_zhh_jm(self._pol_e, self._pol_p, self._z_lepton_mode,
                                            momenta[chunk], jm_zhh[chunk],
                                            self._propagator_type, self._higgs_width)
                    me_zzh = calc_me_zzh_jm(self._pol_e, self._pol_p, self._z_lepton_mode, self.PDG_TO_LCME_MODE[5],
                                            momenta[chunk], jm_zzh[chunk],
                                            self._propagator_type, self._higgs_width)

                    with np.errstate(divide='ignore', invalid='ignore'):
                        output_all[idx[chunk]] = me_zhh / me_zzh

            # assign and save to HDF5
            store[self._output_name] = output_all
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