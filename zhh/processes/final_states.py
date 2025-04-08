from .FinalState import FinalState
import numpy as np
from .ProcessCategories import ProcessCategories

fs_llHH = FinalState('llHH', lambda fs_counts: (
    np.isin(fs_counts['process'], [ProcessCategories.e1e1hh, ProcessCategories.e2e2hh, ProcessCategories.e3e3hh])))
fs_llHH_llbbbb = FinalState('llHH(llbbbb)', lambda fs_counts: (fs_counts['n_b_from_higgs'] == 4) & (
    np.isin(fs_counts['process'], [ProcessCategories.e1e1hh, ProcessCategories.e2e2hh, ProcessCategories.e3e3hh])))
fs_llqqH = FinalState('llqqH', lambda fs_counts: (
    np.isin(fs_counts['process'], [ProcessCategories.e1e1qqh, ProcessCategories.e2e2qqh, ProcessCategories.e3e3qqh])))

class FINAL_STATES:
    llHH = fs_llHH
    llHH_llbbbb = fs_llHH_llbbbb,
    llqqH = fs_llqqH
    
    
FinalStates = FINAL_STATES()