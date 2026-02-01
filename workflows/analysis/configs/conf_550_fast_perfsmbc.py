from .conf_550_fast_base import define_configs_550_fast

define_configs_550_fast('fast-perfsmbc', {
    'analysis_steering.CALO_TREATMENT': 'PERF',
    'analysis_steering.FULL_TRUTHLINK': True,
    'global_generation_steering.PRIMARY_VERTEX_SIM': True
},
sgv_executable_cfg='$SGV_DIR/tests_beamcal_500GeV/usesgvlcio.exe',
sgv_steering_file_src_cfg='$SGV_DIR/tests_beamcal_500GeV/sgv.steer')