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
    'paired_lepton_type': 'I',
    
    # vv
    'zhh_mhh': 'f',
}


class PreselectionSummary(MixedLazyTablelike):
    cache:dict[str, np.ndarray] = {}
    
    def __init__(self, tree:ur.TTree, preselection:str, final_states:bool=True, prop_prefix:str='', cached:bool=False):
        r_size = tree.num_entries
        super().__init__(r_size)
        
        presel = preselection[:2]
        
        def fetch(key:str, loc:str|None=None):
            if loc is None:
                loc = key
                
            if cached:
                if key in self.cache:
                    return self.cache[key]
                else:
                    self.cache[key] = np.array(tree[loc].array(), dtype=dtypes[key])
                    return self.cache[key]                
            else:
                # make dtype default to float
                return np.array(tree[loc].array(), dtype=dtypes[key] if key in dtypes else 'f')
            
        self._defaultHandler = fetch
        
        # writable
        self['id'] = np.arange(r_size, dtype=dtypes['id'])
        self['event_category'] = np.array(tree['event_category'].array(), dtype='B')
        
        # writable; attached by AnalysisChannel.weight()
        self['pid'] = np.nan*np.ones(r_size, dtype='I')
        self['weight'] = np.nan*np.ones(r_size, dtype='f')
        
        # readonly
        self['process'] = lambda: fetch('process')
        self['pol_code'] = lambda: fetch('pol_code', 'polarization_code')
        
        self['event'] = lambda: np.array(tree['event'].array(), dtype='I')
            
        if final_states:
            from .PreselectionAnalysis import fs_columns
            
            self['Nb_from_H'] = lambda: fetch('n_b_from_higgs')
            
            def fs_counts(column:int|None):
                if column is not None:
                    return np.array(tree['final_state_counts'][1].array()[:, column], dtype='B')
                else:
                    return np.array(tree['final_state_counts'][1].array(), dtype='B')
            
            def attach_fs_counts(obj, attr, column:int):
                obj[attr] = lambda: fs_counts(column)
            
            for i in range(len(fs_columns)):
                attach_fs_counts(self, fs_columns[i], i)
            
            # final state counts
            self['fsc'] = lambda: fs_counts(None)
            
        self['thrust'] = lambda: fetch('thrust', f'{prop_prefix}thrust')
        self['evis'] = lambda: fetch('evis', f'{prop_prefix}evis')
        self['ptmiss'] = lambda: fetch('ptmiss', f'{prop_prefix}ptmiss')
        self['m_miss'] = lambda: fetch('m_miss', f'{prop_prefix}m_miss')
        self['nisoleptons'] = lambda: fetch('nisoleptons', f'{prop_prefix}nisoleptons')
        
        # NEW: explicit definition of properties inside TTrees that are of type float are no more necessary
        #self['zhh_mh1'] = lambda: fetch('zhh_mh1', f'{prop_prefix}zhh_mh1')
        #self['zhh_mh2'] = lambda: fetch('zhh_mh2', f'{prop_prefix}zhh_mh2')
        
        if presel == 'll':
            #lepTypes = tree['lepTypes'].array()
            #pass_ltype11 = np.sum(np.abs(lepTypes) == 11, axis=1) == 2
            #pass_ltype13 = np.sum(np.abs(lepTypes) == 13, axis=1) == 2
            #self['ll_dilepton_type'] = pass_ltype11*11 + pass_ltype13*13
            self['dilepton_type'] = lambda: fetch('dilepton_type', f'{prop_prefix}paired_lep_type')
            self['mzll'] = lambda: fetch('mzll', f'{prop_prefix}mzll')
            self['mz_pre_pairing'] = lambda: fetch('mz_pre_pairing', f'{prop_prefix}mzll_pre_pairing')
            
        elif presel == 'vv':
            self['mhh'] = lambda: fetch('mhh', f'{prop_prefix}m_invjet')
        
        self['bmax1'] = lambda: fetch('bmax1', f'{prop_prefix}bmax1')
        self['bmax2'] = lambda: fetch('bmax2', f'{prop_prefix}bmax2')
        self['bmax3'] = lambda: fetch('bmax3', f'{prop_prefix}bmax3')
        self['bmax4'] = lambda: fetch('bmax4', f'{prop_prefix}bmax4')

        self['sumBTags'] = lambda: ( fetch('bmax1') + fetch('bmax2') + fetch('bmax3') + fetch('bmax4') )
        
        # old names for compatability:
        #self[f'{presel}_mh1'] = lambda: self['mh1']
        #self[f'{presel}_mh2'] = lambda: self['mh2']
        
        #self[f'{presel}_bmax1'] = lambda: self['bmax1']
        #self[f'{presel}_bmax2'] = lambda: self['bmax2']
        #self[f'{presel}_bmax3'] = lambda: self['bmax3']
        #self[f'{presel}_bmax4'] = lambda: self['bmax4']

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

def parse_final_state_counts(presel:PreselectionSummary)->FinalStateCounts:
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
