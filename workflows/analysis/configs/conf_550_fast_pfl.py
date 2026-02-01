from .conf_550_fast_base import define_configs_550_fast

define_configs_550_fast('fast-pfl', {
    'analysis_steering.CALO_TREATMENT': 'PFL ',
    'analysis_steering.FULL_TRUTHLINK': True,
})