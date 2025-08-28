from .Cuts import Cut, EqualCut, WindowCut, GreaterThanEqualCut, LessThanEqualCut, ValueCut
from collections.abc import Sequence

def zhh_cuts(hypothesis:str,
             disable_cuts:list[str]=[])->Sequence[ValueCut]:
    
    if hypothesis.lower() == 'eebbbb':
        cuts = [GreaterThanEqualCut('nisoelectrons', 2, label='IsoElectrons'),
                EqualCut('pairedLepType', 11, label='Paired Lepton Type'),
                WindowCut('fit4C_mz', 20., 150., label=r'm_{ll}^{4C}'),
                WindowCut('fit4C_mh1', 50., 200., label=r'm_{H1}^{4C}'),
                WindowCut('fit4C_mh2', 50., 200., label=r'm_{H2}^{4C}'),
                LessThanEqualCut('ptmiss', 70., label=r'p_{t}^{miss}'),
                LessThanEqualCut('thrust', 0.9, label='thrust'),
                GreaterThanEqualCut('sumBTags', 1.0, label='sum b-tags')]
    
    elif False and hypothesis.lower() == 'μμbbbb':
        cuts = [GreaterThanEqualCut('nisomuons', 2, label='IsoMuons'),
                EqualCut('pairedLepType', 13, label='Paired Lepton Type'),
                WindowCut('mzll', 20., 150., label=r'm_{ll}'),
                WindowCut('zhh_mh1', 50., 200., label=r'm_{H1}'),
                WindowCut('zhh_mh2', 50., 200., label=r'm_{H2}'),
                LessThanEqualCut('ptmiss', 70., label=r'p_{t}^{miss}'),
                LessThanEqualCut('thrust', 0.9, label='thrust'),
                GreaterThanEqualCut('sumBTags', 1.0, label='sum b-tags')]
        
    elif hypothesis.lower() == 'μμbbbb': # μμbbbb_4c
        cuts = [GreaterThanEqualCut('nisomuons', 2, label='IsoMuons'),
                EqualCut('pairedLepType', 13, label='Paired Lepton Type'),
                WindowCut('fit4C_mz', 20., 150., label=r'm_{ll}^{4C}'),
                WindowCut('fit4C_mh1', 50., 200., label=r'm_{H1}^{4C}'),
                WindowCut('fit4C_mh2', 50., 200., label=r'm_{H2}^{4C}'),
                LessThanEqualCut('ptmiss', 70., label=r'p_{t}^{miss}'),
                LessThanEqualCut('thrust', 0.9, label='thrust'),
                GreaterThanEqualCut('sumBTags', 1.0, label='sum b-tags')]
        
    elif hypothesis.lower() == 'µµHH_4c':
        cuts = [GreaterThanEqualCut('nisomuons', 2, label='IsoMuons'),
                EqualCut('pairedLepType', 13, label='Paired Lepton Type'),
                WindowCut('fit4C_mz', 20., 150., label=r'm_{ll}^{4C}'),
                WindowCut('fit4C_mh1', 50., 200., label=r'm_{H1}^{4C}'),
                WindowCut('fit4C_mh2', 50., 200., label=r'm_{H2}^{4C}'),
                LessThanEqualCut('ptmiss', 70., label=r'p_{t}^{miss}'),
                LessThanEqualCut('thrust', 0.9, label='thrust')] 
                 
    elif hypothesis[:2].lower() == 'vv':
        cuts = [EqualCut('nisoleptons', 0, label='IsoLeptons'),
                WindowCut('zhh_mh1', 60., 180., label=r'm_{H1}'),
                WindowCut('zhh_mh2', 60., 180., label=r'm_{H2}'),
                WindowCut('ptmiss', 10., 180., label=r'p_{t}^{miss}'),
                LessThanEqualCut('thrust', 0.9, label='thrust'),
                LessThanEqualCut('evis', 400., label=r'E_{vis}'),
                GreaterThanEqualCut('mhh', 220., label=r'm_{HH}'),
                GreaterThanEqualCut('bmax3', 0.2)]
        
    elif hypothesis[:2].lower() == 'qq':
        cuts = [EqualCut('nisoleptons', 0, label='IsoLeptons'),
                GreaterThanEqualCut('qq_bmax4', 0.16, label='bmax4'),
                WindowCut('zhh_mh1', 60., 180., label=r'm_{H1}'),
                WindowCut('zhh_mh2', 60., 180., label=r'm_{H2}'),
                LessThanEqualCut('ptmiss', 70., label=r'p_{t}^{miss}'),
                LessThanEqualCut('thrust', 0.9, label='thrust')]
        
    else:
        raise NotImplementedError(f'Unknown hypothesis <{hypothesis}>')
    
    for to_remove in disable_cuts:
        cuts = [cut for cut in cuts if cut.quantity != to_remove]
        
    return cuts
