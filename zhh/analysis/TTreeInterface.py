from ..util.LazyTablelike import MixedLazyTablelike
import numpy as np
import uproot as ur
from dataclasses import dataclass

dtypes = {
    'id': 'I',
    'process':  'I', # H=np.uint16
    'pid': 'I',
    'pol_code': 'B', # np.uint8
    'event': 'I', # max. encountered: 15 797 803 << 4 294 967 295 (max of uint32)
    'event_category': 'B', # np.uint8
    'n_b_from_higgs': 'B',
    
    'is_sig': '?',
    'is_bkg': '?',
    
    'thrust': 'f',
    'evis': 'f',
    'ptmiss': 'f',
    'm_miss': 'f',
    'nisoleptons': 'B',
    'xx_paired_isoleptype': 'B',
    
    'passed': 'B',
    'weight': 'f',
    
    'bmax1': 'f',
    'bmax2': 'f',
    'bmax3': 'f',
    'bmax4': 'f',
    
    'zhh_mh1': 'f',
    'zhh_mh2': 'f',
    
    # ll
    'dilepton_type': 'B',
    'mzll': 'f',
    'mz_pre_pairing': 'f',
    
    # vv
    'zhh_mhh': 'f',
}


class TTreeInterface(MixedLazyTablelike):
    cache:dict[str, np.ndarray] = {}
    
    def __init__(self, tree:ur.TTree, final_states:bool=True, prop_prefix:str='', cached:bool=False):
        r_size = tree.num_entries
        super().__init__(r_size)
        
        def fetch(key:str, loc:str|None=None):
            if loc is None:
                loc = prop_prefix + key
                
            if cached:
                if key in self.cache:
                    return self.cache[key]
                else:
                    self.cache[key] = np.array(tree[loc].array(), dtype=dtypes[key] if key in dtypes else 'f')
                    return self.cache[key]
            else:
                # make dtype default to float
                return np.array(tree[loc].array(), dtype=dtypes[key] if key in dtypes else 'f')
        
        # this attaches the properties of the TTree 
        self._defaultHandler = fetch
        
        # writable
        self['id'] = np.arange(r_size, dtype=dtypes['id'])
        self['event_category'] = np.array(tree['event_category'].array(), dtype='B')
        
        # writable; attached by AnalysisChannel.weight()
        self['pid'] = np.nan*np.ones(r_size, dtype='I')
        self['weight'] = np.nan*np.ones(r_size, dtype='f')
        
        # readonly
        self['process'] = lambda intf: fetch('process')
        self['pol_code'] = lambda intf: fetch('pol_code', 'polarization_code')
        
        self['event'] = lambda intf: np.array(tree['event'].array(), dtype='I')
            
        if final_states:
            from .PreselectionAnalysis import fs_columns
            
            self['Nb_from_H'] = lambda intf: fetch('n_b_from_higgs')
            
            def fs_counts(column:int|None):
                if column is not None:
                    return np.array(tree['final_state_counts'][1].array()[:, column], dtype='B')
                else:
                    return np.array(tree['final_state_counts'][1].array(), dtype='B')
            
            def attach_fs_counts(obj, attr, column:int):
                obj[attr] = lambda intf: fs_counts(column)
            
            for i in range(len(fs_columns)):
                attach_fs_counts(self, fs_columns[i], i)
            
            # final state counts
            self['fsc'] = lambda intf: fs_counts(None)
        
        # NEW: explicit definition of properties inside TTrees that are of type float are no more necessary
        #self['zhh_mh1'] = lambda intf: fetch('zhh_mh1', f'{prop_prefix}zhh_mh1')
        #self['zhh_mh2'] = lambda intf: fetch('zhh_mh2', f'{prop_prefix}zhh_mh2')
        
        self['mz_pre_pairing'] = lambda intf: fetch('mz_pre_pairing', f'{prop_prefix}mzll_pre_pairing')
        self['mhh'] = lambda intf: fetch('invJetMass', f'{prop_prefix}invJetMass')
        self['sumBTags'] = lambda intf: ( fetch('bmax1') + fetch('bmax2') + fetch('bmax3') + fetch('bmax4') )
        
        # old names for compatability:
        #self[f'{presel}_mh1'] = lambda intf: self['mh1']
        #self[f'{presel}_mh2'] = lambda intf: self['mh2']
        
        #self[f'{presel}_bmax1'] = lambda intf: self['bmax1']
        #self[f'{presel}_bmax2'] = lambda intf: self['bmax2']
        #self[f'{presel}_bmax3'] = lambda intf: self['bmax3']
        #self[f'{presel}_bmax4'] = lambda intf: self['bmax4']

@dataclass
class FinalStateCounts:
    n_d: np.ndarray
    n_u: np.ndarray
    n_s: np.ndarray
    n_c: np.ndarray
    n_b: np.ndarray
    n_t: np.ndarray
    n_q: np.ndarray
    
    n_ve: np.ndarray
    n_vmu: np.ndarray
    n_vtau: np.ndarray
    n_neutral_lep: np.ndarray
    
    n_e: np.ndarray
    n_mu: np.ndarray
    n_tau: np.ndarray
    n_charged_lep: np.ndarray
    
    n_b_from_higgs: np.ndarray

def parse_final_state_counts(presel:TTreeInterface)->FinalStateCounts:
    from zhh import PDG2FSC
    
    presel.resetView()
    fsc = presel['fsc']
    n_b_from_higgs = presel['Nb_from_H']
    
    n_d = fsc[:, PDG2FSC.d]
    n_u = fsc[:, PDG2FSC.u]
    n_s = fsc[:, PDG2FSC.s]
    n_c = fsc[:, PDG2FSC.c]
    n_b = fsc[:, PDG2FSC.b]
    n_t = fsc[:, PDG2FSC.t]
    n_q = n_d + n_u + n_s + n_c + n_b + n_t
    
    n_ve = fsc[:, PDG2FSC.v1]
    n_vmu = fsc[:, PDG2FSC.v2]
    n_vtau = fsc[:, PDG2FSC.v3]
    n_neutral_lep = n_ve + n_vmu + n_vtau
    
    n_e = fsc[:, PDG2FSC.e1]
    n_mu = fsc[:, PDG2FSC.e2]
    n_tau = fsc[:, PDG2FSC.e3]
    n_charged_lep = n_e + n_mu + n_tau
    
    return FinalStateCounts(
        n_d=n_d,
        n_u=n_u,
        n_s=n_s,
        n_c=n_c,
        n_b=n_b,
        n_t=n_t,
        n_q=n_q,
        
        n_ve=n_ve,
        n_vmu=n_vmu,
        n_vtau=n_vtau,
        n_neutral_lep=n_neutral_lep,
        
        n_e=n_e,
        n_mu=n_mu,
        n_tau=n_tau,
        n_charged_lep=n_charged_lep,
        n_b_from_higgs=n_b_from_higgs
    )
