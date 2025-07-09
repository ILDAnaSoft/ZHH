from .Cuts import Cut, EqualCut, WindowCut, GreaterThanEqualCut, LessThanEqualCut
from collections.abc import Sequence

def zhh_cuts(hypothesis:str,
             additional:bool=True,
             b_tagging:bool=True,
             mH:float=125.,
             mZ:float=91.2,
             disable_cuts:list[str]=[])->Sequence[Cut]:
    
    if hypothesis[:2].lower() == 'll':
        cuts = [GreaterThanEqualCut('nisoleptons', 2, label='nisoleptons'),
                WindowCut('mzll', mZ, 80, center=True, label=r'm_{Z}')]
        
        if additional:
            cuts += [WindowCut('zhh_mh1', 60., 180., label=r'm_{H1}'),
                     WindowCut('zhh_mh1', 60., 180., label=r'm_{H2}'),
                     LessThanEqualCut('ptmiss', 70., label=r'p_{t}^{miss}'),
                     LessThanEqualCut('thrust', 0.9, label='thrust'),]
        
    elif hypothesis[:2].lower() == 'vv':
        cuts = [EqualCut('nisoleptons', 0, label='nisoleptons'),
                WindowCut('zhh_mh1', 60., 180., label=r'm_{H1}'),
                WindowCut('zhh_mh2', 60., 180., label=r'm_{H2}'),
                WindowCut('ptmiss', 10., 180., label=r'p_{t}^{miss}'),
                LessThanEqualCut('thrust', 0.9, label='thrust'),
                LessThanEqualCut('evis', 400., label=r'E_{vis}'),
                GreaterThanEqualCut('mhh', 220., label=r'm_{HH}')]
        
        if b_tagging:
            cuts += [GreaterThanEqualCut('bmax3', 0.2)]
        
    elif hypothesis[:2].lower() == 'qq':
        cuts = [EqualCut('nisoleptons', 0)]
        
        if b_tagging:
            cuts += [GreaterThanEqualCut('qq_bmax4', 0.16, label='bmax4')]
        
        if additional:
            cuts += [WindowCut('zhh_mh1', 60., 180., label=r'm_{H1}'),
                     WindowCut('zhh_mh2', 60., 180., label=r'm_{H2}'),
                     LessThanEqualCut('ptmiss', 70., label=r'p_{t}^{miss}'),
                     LessThanEqualCut('thrust', 0.9, label='thrust')]
        
    else:
        raise NotImplementedError(f'Unknown hypothesis <{hypothesis}>')
    
    for to_remove in disable_cuts:
        cuts = [cut for cut in cuts if cut.quantity != to_remove]
        
    return cuts