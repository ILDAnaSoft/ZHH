from ..util.LazyTablelike import LazyTablelike
import numpy as np
import uproot as ur

class FinalStateDict(LazyTablelike):
    process: np.ndarray
    n_d: np.ndarray
    n_u: np.ndarray
    n_s: np.ndarray
    n_c: np.ndarray
    n_b: np.ndarray
    n_t: np.ndarray
    n_b_from_higgs: np.ndarray
    n_higgs: np.ndarray
    n_q: np.ndarray
    n_v: np.ndarray
    n_e: np.ndarray
    n_mu: np.ndarray
    n_tau: np.ndarray
    n_charged_lep: np.ndarray
    
    def __init__(self, tree:ur.TTree):
        super().__init__(tree.num_entries)
        
        counts = lambda: np.array(tree['final_state_counts.second'].array(), dtype='B')
        
        self['process'] = lambda : np.array(tree['process'].array(), dtype='I')
        self['n_d'] = lambda : counts()[:, 0]
        self['n_u'] = lambda : counts()[:, 1]
        self['n_s'] = lambda : counts()[:, 2]
        self['n_c'] = lambda : counts()[:, 3]
        self['n_b'] = lambda : counts()[:, 4]
        self['n_t'] = lambda : counts()[:, 5]
        self['n_b_from_higgs'] = lambda : np.array(tree['n_b_from_higgs'].array(), dtype='B')
        self['n_higgs'] = lambda : np.array(tree['n_higgs'].array(), dtype='B')
        self['n_q'] = lambda : (
            ct := counts(),
            ct[:, 0] + ct[:, 1] + ct[:, 2] + ct[:, 3] + ct[:, 4] + ct[:, 5]          
        )
        self['n_v'] = lambda : (
            ct := counts(),
            ct[:, 7] + ct[:, 9] + ct[:, 11]
        )
        self['n_e'] = lambda : counts()[:, 6]
        self['n_mu'] = lambda : counts()[:, 8]
        self['n_tau'] = lambda : counts()[:, 10]
        self['n_charged_lep'] = lambda : (
            ct := counts(),
            ct[:, 6] + ct[:, 8] + ct[:, 10]
        )
    