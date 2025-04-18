class EVENT_CATEGORY_TRUE:
    OTHER = 0

    # LEPTONIC
    OTHER_LL = 10
    llHH = 11 # // llbbbb (ZHH signal)
    
    # START Only used in analysis (not in Marlin processor)
    eeHH = 111
    µµHH = 112
    𝜏𝜏HH = 113
    
    eeHHbbbb = 114
    µµHHbbbb = 115
    𝜏𝜏HHbbbb = 116
    # END
    
    eebb = 12
    µµbb = 13
    𝜏𝜏bb = 14
    llbbbb = 15
    llqqH = 16
    ll = 17
    llll = 18
    llqq = 19
    llvv = 20
    eeWW = 21
    llWW = 22

    # NEUTRINO
    OTHER_VV = 30
    vvHH = 31 # vvbbbb (ZHH signal)
    
    # START Only used in analysis (not in Marlin processor)
    v1v1HH = 131
    v23v23HH = 132
    
    v1v1HHbbbb = 133
    v23v23HHbbbb = 134
    # END
    
    vvbb = 32
    vvbbbb = 33
    vvqqH = 34
    vv = 35
    vvqq = 36
    vvWW = 37

    # HADRONIC
    OTHER_QQ = 50
    qqHH = 51 # qqbbbb (ZHH signal)
    
    # START Only used in analysis (not in Marlin processor)
    qqHHbbbb = 151
    bbHHbbbb = 152 
    # END

    qqqqH = 52
    qqbbbb = 53
    bbbb = 54
    ttZ = 55
    ttbb = 56
    qq = 57
    qqqq = 58
    bbbbbb = 59

    # ttbar -> lvbbqq [t->Wb, W->lv/qq, b->bb]
    # so far not accounted: ttbar -> llvvbb (two leptonically decaying W bosons)
    # reason: https://tikz.net/sm_decay_piechart/
    # W -> qqbar 67%; W -> lv 33%
    # => 2xW -> qqbar 67% * 67% = 44.89% (two hadronic decays)
    # => 2xW -> lv 33% * 33% = 10.89% (two leptonic decays)
    # rest: 44.22% (one hadronic, one leptonic decay)
    OTHER_TTBAR = 70
    evbbqq = 71
    µvbbqq = 72
    𝜏vbbqq = 73
    
    evbbcs = 74
    µvbbcs = 75
    𝜏vbbcs = 76

    evbbud = 77
    µvbbud = 78
    𝜏vbbud = 79

    # tt/WWZ -> bbqqqq
    # for tt: tt -> bbqqqq : 2x [t->Wb; W->qq]
    # for WWZ: WWZ -> bbqqqq : 2x [W->qq; Z->bb]
    OTHER_FULL_HADRONIC = 80
    bbqqqq = 81
    bbcssc = 82
    bbuddu = 83
    bbcsdu = 84

    OTHER_EVENTS = 90
    f5_any = 91

    f6_yyyyZ = 95
    f6_xxWW = 96
    f6_xxxxZ = 97
    
    OTHER_F6 = 98
    
    def __init__(self) -> None:
        self.map = { key:value for key, value in EVENT_CATEGORY_TRUE.__dict__.items() if not key.startswith('__') and not callable(key)}
        self.inverted = { v: k for k, v in self.map.items() }
    
EventCategories = EVENT_CATEGORY_TRUE()