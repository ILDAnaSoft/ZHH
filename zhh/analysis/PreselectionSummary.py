from ..util.LazyTablelike import MixedLazyTablelike
import numpy as np
import uproot as ur
from .PreselectionAnalysis import fs_columns
from typing import TypedDict

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
    'e_vis': 'f',
    'pt_miss': 'f',
    'invmass_miss': 'f',
    'nisoleps': 'B',
    'xx_paired_isoleptype': 'B',
    
    'passed': 'B',
    'weight': 'f',
    
    'bmax1': 'f',
    'bmax2': 'f',
    'bmax3': 'f',
    'bmax4': 'f',
    
    # ll
    'dilepton_type': 'B',
    'mz': 'f',
    'mz_pre_pairing': 'f',
    
    # vv
    'mhh': 'f',
    
    # qq
    'mh1': 'f',
    'mh2': 'f',
}


class PreselectionSummary(MixedLazyTablelike):
    def __init__(self, tree:ur.TTree, preselection:str, final_states:bool=True, prop_prefix:str=''):
        r_size = tree.num_entries
        super().__init__(r_size)        
        
        presel = preselection[:2]
        
        def fetch(key:str, loc:str|None=None):
            if loc is None:
                loc = key
                
            return np.array(tree[loc].array(), dtype=dtypes[key])
        
        # writable
        self['id'] = np.arange(r_size, dtype=dtypes['id'])
        self['event_category'] = np.array(tree['event_category'].array(), dtype='B')
        
        # readonly
        self['process'] = lambda: fetch('process')
        self['pol_code'] = lambda: fetch('pol_code', 'polarization_code')
        
        self['event'] = lambda: np.array(tree['event'].array(), dtype='I')
            
        if final_states:
            fs_counts = lambda: tree['final_state_counts'][1].array()
            self['Nb_from_H'] = lambda: fetch('n_b_from_higgs')
            
            for i in range(len(fs_columns)):
                self[fs_columns[i]] = lambda: np.array(fs_counts()[:, i], dtype='B')
        
        self['thrust'] = lambda: fetch('thrust', f'{prop_prefix}thrust')
        self['e_vis'] = lambda: fetch('e_vis', f'{prop_prefix}evis')
        self['pt_miss'] = lambda: fetch('pt_miss', f'{prop_prefix}ptmiss')
        self['invmass_miss'] = lambda: fetch('invmass_miss', f'{prop_prefix}m_miss')
        self['nisoleps'] = lambda: fetch('nisoleps', f'{prop_prefix}nisoleptons')
        
        # keep {presel}_mhi for compatability; NEW way should be WITHOUT preselection
        self['mh1'] = lambda: fetch('mh1', f'{prop_prefix}zhh_mh1')
        self['mh2'] = lambda: fetch('mh2', f'{prop_prefix}zhh_mh2')
        
        if presel == 'll':
            #lepTypes = tree['lepTypes'].array()
            #pass_ltype11 = np.sum(np.abs(lepTypes) == 11, axis=1) == 2
            #pass_ltype13 = np.sum(np.abs(lepTypes) == 13, axis=1) == 2
            #self['ll_dilepton_type'] = pass_ltype11*11 + pass_ltype13*13
            self['dilepton_type'] = lambda: fetch('dilepton_type', f'{prop_prefix}paired_lep_type')
            self['mz'] = lambda: fetch('mz', f'{prop_prefix}mzll')
            self['mz_pre_pairing'] = lambda: fetch('mz_pre_pairing', f'{prop_prefix}mzll_pre_pairing')
            
        elif presel == 'vv':
            self['mhh'] = lambda: fetch('mhh', f'{prop_prefix}m_invjet')
        
        self['bmax1'] = lambda: fetch('bmax1', f'{prop_prefix}bmax1')
        self['bmax2'] = lambda: fetch('bmax2', f'{prop_prefix}bmax2')
        self['bmax3'] = lambda: fetch('bmax3', f'{prop_prefix}bmax3')
        self['bmax4'] = lambda: fetch('bmax4', f'{prop_prefix}bmax4')
        
        # old names for compatability:
        self[f'{presel}_mh1'] = lambda: self['mh1']
        self[f'{presel}_mh2'] = lambda: self['mh2']
        
        self[f'{presel}_bmax1'] = lambda: self['bmax1']
        self[f'{presel}_bmax2'] = lambda: self['bmax2']
        self[f'{presel}_bmax3'] = lambda: self['bmax3']
        self[f'{presel}_bmax4'] = lambda: self['bmax4']