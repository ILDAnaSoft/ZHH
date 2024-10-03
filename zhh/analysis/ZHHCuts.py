from .Cuts import Cut, EqualCut, WindowCut, GreaterThanEqualCut, LessThanEqualCut

def zhh_cuts(hypothesis:str,
             additional:bool=True,
             b_tagging:bool=True,
             mH:float=125.,
             mZ:float=91.2,):
    
    if hypothesis == 'llHH':
        cuts = [GreaterThanEqualCut('xx_nisoleps', 2),
                WindowCut('ll_mz', mZ, 40, center=True)]
        
        if additional:
            cuts += [WindowCut('ll_mh1', 60., 180.),
                     WindowCut('ll_mh2', 60., 180.),
                     LessThanEqualCut('xx_pt_miss', 70.),
                     LessThanEqualCut('xx_thrust', 0.9)]
        
    elif hypothesis == 'vvHH':
        cuts = [EqualCut('xx_nisoleps', 0)]
        
        if b_tagging:
            cuts += [GreaterThanEqualCut('vv_bmax3', 0.2)]
        
        cuts += [WindowCut('vv_mh1', 60., 180.),
                 WindowCut('vv_mh2', 60., 180.),
                 WindowCut('xx_pt_miss', 10., 180.),
                 LessThanEqualCut('xx_thrust', 0.9),
                 LessThanEqualCut('xx_e_vis', 400.),
                 GreaterThanEqualCut('vv_mhh', 220.)]
        
    elif hypothesis == 'qqHH':
        cuts = [EqualCut('xx_nisoleps', 0)]
        
        if b_tagging:
            cuts += [GreaterThanEqualCut('qq_bmax4', 0.16)]
        
        if additional:
            cuts += [WindowCut('qq_mh1', 60., 180.),
                     WindowCut('qq_mh2', 60., 180.),
                     LessThanEqualCut('xx_pt_miss', 70.),
                     LessThanEqualCut('xx_thrust', 0.9)]
        
    return cuts