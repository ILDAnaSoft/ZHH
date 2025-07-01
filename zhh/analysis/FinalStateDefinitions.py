from .PreselectionSummary import FinalStateCounts, PreselectionSummary
from .AnalysisChannel import AnalysisChannel
from collections.abc import Callable
from ..processes.ProcessCategories import ProcessCategories
import numpy as np

FinalStateDefinition = Callable[[AnalysisChannel, FinalStateCounts], np.ndarray]

# define llbb
define_eebb:FinalStateDefinition = lambda ac, fsc: (fsc.n_e   == 2) & (fsc.n_b == 2)
define_µµbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_mu  == 2) & (fsc.n_b == 2)
define_ττbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_tau == 2) & (fsc.n_b == 2)

# categorize l2q4
define_lvqqqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 1) & (fsc.n_neutral_lep == 1) & (fsc.n_q == 4) & (fsc.n_b == 0)
define_lvbbqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 1) & (fsc.n_neutral_lep == 1) & (fsc.n_q == 4) & (fsc.n_b == 2)
define_evbbqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_e           == 1) & (fsc.n_neutral_lep == 1) & (fsc.n_q == 4) & (fsc.n_b == 2)
define_µvbbqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_mu          == 1) & (fsc.n_neutral_lep == 1) & (fsc.n_q == 4) & (fsc.n_b == 2)
define_τvbbqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_tau         == 1) & (fsc.n_neutral_lep == 1) & (fsc.n_q == 4) & (fsc.n_b == 2)
define_lvbbbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 1) & (fsc.n_neutral_lep == 1) & (fsc.n_q == 4) & (fsc.n_b == 4)
define_llqqqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 2) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 4) & (fsc.n_b == 0)
define_llbbqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 2) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 4) & (fsc.n_b == 2)
define_llbbbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 2) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 4) & (fsc.n_b == 4)
define_vvqqqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 0) & (fsc.n_neutral_lep == 2) & (fsc.n_q == 4) & (fsc.n_b == 0)
define_vvbbqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 0) & (fsc.n_neutral_lep == 2) & (fsc.n_q == 4) & (fsc.n_b == 2)

# categorize q6
define_bbqqqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 0) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 6) & (fsc.n_b == 2)
define_bbbbqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 0) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 6) & (fsc.n_b == 4)
define_bbbbbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 0) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 6) & (fsc.n_b == 6)

# categorize llhh
define_llhh:FinalStateDefinition     = lambda ac, fsc: np.isin(ac.getPreselection()['process'], [ProcessCategories.e1e1hh, ProcessCategories.e2e2hh, ProcessCategories.e3e3hh])
define_eeHHbbbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_e == 2)   & (fsc.n_charged_lep == 2) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 4) & (fsc.n_b_from_higgs == 4)
define_µµHHbbbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_mu == 2)  & (fsc.n_charged_lep == 2) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 4) & (fsc.n_b_from_higgs == 4)
define_ττHHbbbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_tau == 2) & (fsc.n_charged_lep == 2) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 4) & (fsc.n_b_from_higgs == 4)

define_llhh_llbbbb:FinalStateDefinition = lambda ac, fsc: np.logical_or.reduce( # required correct naming in registerEventCategory
    (ac.getCategoryMask('eeHHbbbb'), ac.getCategoryMask('µµHHbbbb'), ac.getCategoryMask('ττHHbbbb')))
define_llqqh:FinalStateDefinition = lambda ac, fsc: np.isin(ac.getPreselection()['process'], [ProcessCategories.e1e1qqh, ProcessCategories.e2e2qqh, ProcessCategories.e3e3qqh])
