from .Cuts import Cut, EqualCut, WindowCut, GreaterThanEqualCut, LessThanEqualCut

def zhh_cuts(hypothesis:str,
             additional:bool=True,
             b_tagging:bool=True,
             mH:float=125.,
             mZ:float=91.2,):
    
    if hypothesis[:2].lower() == 'll':
        cuts = [GreaterThanEqualCut('xx_nisoleps', 2, label='nisoleps'),
                WindowCut('ll_mz', mZ, 40, center=True, label=r'm_{Z}')]
        
        if additional:
            cuts += [WindowCut('ll_mh1', 60., 180., label=r'm_{H1}'),
                     WindowCut('ll_mh2', 60., 180., label=r'm_{H2}'),
                     LessThanEqualCut('xx_pt_miss', 70., label=r'p_{t}^{miss}'),
                     LessThanEqualCut('xx_thrust', 0.9, label='thrust'),]
        
    elif hypothesis[:2].lower() == 'vv':
        cuts = [EqualCut('xx_nisoleps', 0, label='nisoleps'),
                WindowCut('vv_mh1', 60., 180., label=r'm_{H1}'),
                WindowCut('vv_mh2', 60., 180., label=r'm_{H2}'),
                WindowCut('xx_pt_miss', 10., 180., label=r'p_{t}^{miss}'),
                LessThanEqualCut('xx_thrust', 0.9, label='thrust'),
                LessThanEqualCut('xx_e_vis', 400., label=r'E_{vis}'),
                GreaterThanEqualCut('vv_mhh', 220., label=r'm_{HH}')]
        
        if b_tagging:
            cuts += [GreaterThanEqualCut('vv_bmax3', 0.2)]
        
    elif hypothesis[:2].lower() == 'qq':
        cuts = [EqualCut('xx_nisoleps', 0)]
        
        if b_tagging:
            cuts += [GreaterThanEqualCut('qq_bmax4', 0.16, label='bmax4')]
        
        if additional:
            cuts += [WindowCut('qq_mh1', 60., 180., label=r'm_{H1}'),
                     WindowCut('qq_mh2', 60., 180., label=r'm_{H2}'),
                     LessThanEqualCut('xx_pt_miss', 70., label=r'p_{t}^{miss}'),
                     LessThanEqualCut('xx_thrust', 0.9, label='thrust')]
        
    else:
        raise NotImplementedError(f'Unknown hypothesis <{hypothesis}>')
        
    return cuts