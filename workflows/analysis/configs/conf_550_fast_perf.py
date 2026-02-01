from .conf_550_fast_base import define_configs_550_fast

define_configs_550_fast('fast-perf', {
    'analysis_steering.CALO_TREATMENT': 'PERF',
    'analysis_steering.FULL_TRUTHLINK': True,
})