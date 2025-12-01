from .TTreeInterface import FinalStateCounts, TTreeInterface
from .DataSource import DataSource
from collections.abc import Callable
from ..processes.ProcessCategories import ProcessCategories
import numpy as np

FinalStateDefinition = Callable[[DataSource, FinalStateCounts], np.ndarray]

# define llbb
define_eebb:FinalStateDefinition = lambda ac, fsc: (fsc.n_e   == 2)         & (fsc.n_b == 2)
define_μμbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_mu  == 2)         & (fsc.n_b == 2)
define_ττbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_tau == 2)         & (fsc.n_b == 2)
define_lvqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 1) & (fsc.n_q == 2) & (fsc.n_neutral_lep == 1)

# categorize l2q4
define_lvqqqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 1) & (fsc.n_neutral_lep == 1) & (fsc.n_q == 4)
define_lvbbqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 1) & (fsc.n_neutral_lep == 1) & (fsc.n_q == 4) & (fsc.n_b == 2)
define_evbbqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_e           == 1) & (fsc.n_neutral_lep == 1) & (fsc.n_q == 4) & (fsc.n_b == 2)
define_μvbbqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_mu          == 1) & (fsc.n_neutral_lep == 1) & (fsc.n_q == 4) & (fsc.n_b == 2)
define_τvbbqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_tau         == 1) & (fsc.n_neutral_lep == 1) & (fsc.n_q == 4) & (fsc.n_b == 2)
define_lvbbbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 1) & (fsc.n_neutral_lep == 1) & (fsc.n_q == 4) & (fsc.n_b == 4)
define_llqqqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 2) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 4)
define_llbbqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 2) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 4) & (fsc.n_b == 2)
define_llbbbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 2) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 4) & (fsc.n_b == 4)
define_vvqqqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 0) & (fsc.n_neutral_lep == 2) & (fsc.n_q == 4)
define_vvbbqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 0) & (fsc.n_neutral_lep == 2) & (fsc.n_q == 4) & (fsc.n_b == 2)

# categorize q6
define_bbqqqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 0) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 6) & (fsc.n_b == 2)
define_bbbbqq:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 0) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 6) & (fsc.n_b == 4)
define_bbbbbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_charged_lep == 0) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 6) & (fsc.n_b == 6)

# categorize llhh
define_llhh:FinalStateDefinition     = lambda ac, fsc: np.isin(ac.getStore()['process'], [ProcessCategories.e1e1hh, ProcessCategories.e2e2hh, ProcessCategories.e3e3hh])
define_eeHHbbbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_e == 2)   & (fsc.n_charged_lep == 2) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 4) & (fsc.n_b_from_higgs == 4)
define_μμHHbbbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_mu == 2)  & (fsc.n_charged_lep == 2) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 4) & (fsc.n_b_from_higgs == 4)
define_ττHHbbbb:FinalStateDefinition = lambda ac, fsc: (fsc.n_tau == 2) & (fsc.n_charged_lep == 2) & (fsc.n_neutral_lep == 0) & (fsc.n_q == 4) & (fsc.n_b_from_higgs == 4)

define_llhh_llnonbbbb:FinalStateDefinition = lambda ac, fsc: np.isin(ac.getStore()['process'], [ProcessCategories.e1e1hh, ProcessCategories.e2e2hh, ProcessCategories.e3e3hh]) & (fsc.n_charged_lep == 2) & (fsc.n_neutral_lep == 0) & (fsc.n_b_from_higgs != 4)
define_llhh_llbbbb:FinalStateDefinition = lambda ac, fsc: np.logical_or.reduce( # required correct naming in registerEventCategory
    (ac.getCategoryMask('eeHHbbbb'), ac.getCategoryMask('μμHHbbbb'), ac.getCategoryMask('ττHHbbbb')))
define_llqqh:FinalStateDefinition = lambda ac, fsc: np.isin(ac.getStore()['process'], [ProcessCategories.e1e1qqh, ProcessCategories.e2e2qqh, ProcessCategories.e3e3qqh])

# categorize vvhh
define_vvhh:FinalStateDefinition         = lambda ac, fsc: np.isin(ac.getStore()['process'], [ProcessCategories.n1n1hh, ProcessCategories.n23n23hh])
define_vvqqh:FinalStateDefinition        = lambda ac, fsc: np.isin(ac.getStore()['process'], [ProcessCategories.n1n1qqh, ProcessCategories.n23n23qqh])
define_v1v1HHbbbb:FinalStateDefinition   = lambda ac, fsc: np.isin(ac.getStore()['process'], [ProcessCategories.n1n1hh])
define_v23v23HHbbbb:FinalStateDefinition = lambda ac, fsc:  np.isin(ac.getStore()['process'], [ProcessCategories.n23n23hh])

define_vvhh_vvnonbbbb:FinalStateDefinition = lambda ac, fsc: ac.getCategoryMask('vvHHbbbb') & (fsc.n_charged_lep == 2) & (fsc.n_neutral_lep == 0) & (fsc.n_b_from_higgs != 4)
define_vvhh_vvbbbb:FinalStateDefinition = lambda ac, fsc: np.logical_or.reduce((ac.getCategoryMask('v1v1HHbbbb'), ac.getCategoryMask('v23v23HHbbbb')))

# categorize qqhh
define_qqhh:FinalStateDefinition     = lambda ac, fsc: np.isin(ac.getStore()['process'], [ProcessCategories.qqhh])

# categorize ttH + ttZ
define_ttH:FinalStateDefinition     = lambda ac, fsc: ac.getStore()['process'] == ProcessCategories.f8_tth
define_ttZ:FinalStateDefinition     = lambda ac, fsc: ac.getStore()['process'] == ProcessCategories.f8_ttz
define_ttHZ:FinalStateDefinition     = lambda ac, fsc: np.isin(ac.getStore()['process'], [ProcessCategories.f8_tth, ProcessCategories.f8_ttz])

def categorize_4fsl(ac:DataSource):
    from zhh import EventCategories
    
    ac.registerEventCategory('eebb', define_eebb, EventCategories.eebb)
    ac.registerEventCategory('μμbb', define_μμbb, EventCategories.μμbb)
    ac.registerEventCategory('ττbb', define_ττbb, EventCategories.ττbb)
    ac.registerEventCategory('lvqq', define_lvqq, EventCategories.lvqq)

def categorize_2l4q(ac:DataSource):
    from zhh import EventCategories
    
    ac.registerEventCategory('lvqqqq', define_lvqqqq, EventCategories.lvqqqq)
    ac.registerEventCategory('lvbbqq', define_lvbbqq, EventCategories.lvbbqq)
    ac.registerEventCategory('evbbqq', define_evbbqq, None) # EventCategories.evbbqq
    ac.registerEventCategory('μvbbqq', define_μvbbqq, None) # EventCategories.μvbbqq
    ac.registerEventCategory('τvbbqq', define_τvbbqq, None) # EventCategories.τvbbqq
    ac.registerEventCategory('lvbbbb', define_lvbbbb, EventCategories.lvbbbb)
    ac.registerEventCategory('llqqqq', define_llqqqq, EventCategories.llqqqq)
    ac.registerEventCategory('llbbqq', define_llbbqq, None) # EventCategories.llbbqq
    ac.registerEventCategory('llbbbb', define_llbbbb, None) # EventCategories.llbbqq
    ac.registerEventCategory('vvqqqq', define_vvqqqq, EventCategories.vvqqqq)
    ac.registerEventCategory('vvbbqq', define_vvbbqq, None) # EventCategories.vvbbqq

def categorize_6q(ac:DataSource):
    from zhh import EventCategories
    
    ac.registerEventCategory('bbqqqq', define_bbqqqq, EventCategories.bbqqqq)
    ac.registerEventCategory('bbbbqq', define_bbbbqq, EventCategories.bbbbqq)
    ac.registerEventCategory('bbbbbb', define_bbbbbb, EventCategories.bbbbbb)

def categorize_llhh(ac:DataSource):
    from zhh import EventCategories
    
    ac.registerEventCategory('llhh', define_llhh, EventCategories.llHH)
    ac.registerEventCategory('eeHHbbbb', define_eeHHbbbb, EventCategories.eeHHbbbb)
    ac.registerEventCategory('μμHHbbbb', define_μμHHbbbb, EventCategories.μμHHbbbb)
    ac.registerEventCategory('ττHHbbbb', define_ττHHbbbb, EventCategories.ττHHbbbb)
    ac.registerEventCategory('llhh_llbbbb', define_llhh_llbbbb, None)
    ac.registerEventCategory('llhh_llnonbbbb', define_llhh_llnonbbbb, None)
    ac.registerEventCategory('llqqh', define_llqqh, EventCategories.llqqH)

def categorize_vvhh(ac:DataSource):
    from zhh import EventCategories
    
    ac.registerEventCategory('vvhh', define_vvhh, EventCategories.vvHH)
    ac.registerEventCategory('v1v1HHbbbb', define_v1v1HHbbbb, EventCategories.v1v1HHbbbb)
    ac.registerEventCategory('v23v23HHbbbb', define_v23v23HHbbbb, EventCategories.v23v23HHbbbb)
    ac.registerEventCategory('vvhh_vvbbbb', define_vvhh_vvbbbb, None)
    ac.registerEventCategory('vvhh_vvnonbbbb', define_vvhh_vvnonbbbb, None)
    ac.registerEventCategory('vvqqh', define_vvqqh, EventCategories.vvqqH)

def categorize_tthz(ac:DataSource):
    from zhh import EventCategories
    
    ac.registerEventCategory('ttH', define_ttH, EventCategories.ttH)
    ac.registerEventCategory('ttZ', define_ttZ, EventCategories.ttZ)
    ac.registerEventCategory('ttHZ', define_ttHZ, EventCategories.ttHZ)