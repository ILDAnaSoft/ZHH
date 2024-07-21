class EVENT_CATEGORY_TRUE:
    OTHER = 0

    # LEPTONIC
    OTHER_LL = 10
    llHH = 11 # // llbbbb (ZHH signal)

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

    vvbb = 32
    vvbbbb = 33
    vvqqH = 34
    vv = 35
    vvqq = 36
    vvWW = 37

    # HADRONIC
    OTHER_QQ = 50
    qqHH = 51 # qqbbbb (ZHH signal)

    qqqqH = 52
    qqbbbb = 53
    bbbb = 54
    ttZ = 55
    ttbb = 56
    qq = 57
    qqqq = 58

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