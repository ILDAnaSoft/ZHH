def register():
    # skip hep_workflows plugin registration if REPO_ROOT is not on path
    # useful as a workaround as long as cutflow is part of this repo
    from os import environ
    if 'REPO_ROOT' not in environ:
        return

    from workflows.analysis.configs import \
        conf_550_fast_base, \
        conf_550_fast_perf, \
        conf_550_fast_perfsmbc, \
        conf_550_fast_pfl, \
        conf_550_full

    import workflows.analysis.configurations
    import zhh.workflows.e550_hh_sm_ac