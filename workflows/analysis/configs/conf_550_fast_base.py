from analysis.framework import AnalysisConfiguration, zhh_configs
from typing import TYPE_CHECKING
from os import environ, path as osp
import numpy as np
from zhh import glob_exp

if TYPE_CHECKING:
    from analysis.tasks import RawIndex, AbstractIndex

def define_configs_550_fast(suffix:str, sgv_options:dict,
                            sgv_executable_cfg:str='$SGV_DIR/tests/usesgvlcio.exe',
                            sgv_steering_file_src_cfg:str='$SGV_DIR/tests/sgv.steer'):
    
    class Config_550_llhh(AnalysisConfiguration):
        tag = f'550-llhh-{suffix}'
        
        def sgv_inputs(self, fast_sim_task):
            input_files:list[str] = sum(map(glob_exp, [
                '$ILC_PROD_PATH/mc-2020/generated/550-Test/hh/*Pe1e1*.slcio',
                '$ILC_PROD_PATH/mc-2020/generated/550-Test/hh/*Pe2e2*.slcio',
                '$ILC_PROD_PATH/mc-2020/generated/550-Test/hh/*Pe3e3*.slcio'
            ]), [])
            input_files.sort()
            
            input_options = [{
                'global_steering.MAXEV': 999999,
                'global_generation_steering.CMS_ENE': 550,
                'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
                'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
                **sgv_options
            }] * len(input_files)
            
            return input_files, input_options
        
        marlin_globals = {  }
        marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

        sgv_executable = sgv_executable_cfg
        sgv_steering_file_src = sgv_steering_file_src_cfg

    class Config_550_qqhh(AnalysisConfiguration):
        tag = f'550-qqhh-{suffix}'
        
        def sgv_inputs(self, fast_sim_task):
            input_files:list[str] = sum(map(glob_exp, [
                '$ILC_PROD_PATH/mc-2020/generated/550-Test/hh/*Pqqhh*.slcio',
                '$ILC_PROD_PATH/mc-2020/generated/550-Test/hh/*Pqqqqh*.slcio'
            ]), [])
            input_files.sort()
            
            input_options = [{
                'global_steering.MAXEV': 999999,
                'global_generation_steering.CMS_ENE': 550,
                'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
                'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
                **sgv_options
            }] * len(input_files)
            
            return input_files, input_options
        
        marlin_globals = {  }
        marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

        sgv_executable = sgv_executable_cfg
        sgv_steering_file_src = sgv_steering_file_src_cfg

    class Config_550_vvhh(AnalysisConfiguration):
        tag = f'550-vvhh-{suffix}'
        
        def sgv_inputs(self, fast_sim_task):
            input_files:list[str] = sum(map(glob_exp, [
                '$ILC_PROD_PATH/mc-2020/generated/550-Test/hh/*Pn1n1*.slcio',
                '$ILC_PROD_PATH/mc-2020/generated/550-Test/hh/*Pn23n23*.slcio',
            ]), [])
            input_files.sort()
            
            input_options = [{
                'global_steering.MAXEV': 999999,
                'global_generation_steering.CMS_ENE': 550,
                'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
                'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
                **sgv_options
            }] * len(input_files)
            
            return input_files, input_options
        
        marlin_globals = {  }
        marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

        sgv_executable = sgv_executable_cfg
        sgv_steering_file_src = sgv_steering_file_src_cfg

    class Config_550_6q(AnalysisConfiguration):
        tag = f'550-6q-{suffix}'
        
        def sgv_inputs(self, fast_sim_task):
            process_mask_6q = [
                'P6f_xxxxxx',
                'P6f_xxxyyx',
                'P6f_yycyyc',
                'P6f_yycyyu',
                'P6f_yyuyyc',
                'P6f_yyuyyu',
                'P6f_yyyyyy']
            
            input_files:list[str] = []
            for process_mask in process_mask_6q:
                input_files += glob_exp(f'$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/6f/*{process_mask}*.slcio')
            
            # filter out invalid files
            input_files = list(filter(lambda x: 'E550-TDR_ws.P6f_yyuyyu.Gwhizard-3_1_5.eL.pR.I410212.2.slcio' not in x, input_files))
            
            input_options = [{
                'global_steering.MAXEV': 999999,
                'global_generation_steering.CMS_ENE': 550,
                'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
                'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
                **sgv_options
            }] * len(input_files)
            
            return input_files, input_options
        
        marlin_globals = {  }
        marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

        sgv_executable = sgv_executable_cfg
        sgv_steering_file_src = sgv_steering_file_src_cfg

    class Config_550_4fsl(AnalysisConfiguration):
        tag = f'550-4fsl-{suffix}'
        
        custom_statistics = [
            (1., 'zz_sl0')
        ]
        
        def raw_index_requires(self, raw_index_task: 'AbstractIndex'):
            # use the output of 550-4f-{suffix} as input
            
            from workflows.analysis.tasks_sim import FastSimSGV
            from analysis.tasks import RawIndex
            
            fast_sim_dep = FastSimSGV.req(raw_index_task, tag=f'550-4f-{suffix}')
            raw_index_dep = RawIndex.req(raw_index_task, tag=f'550-4f-{suffix}') 
                        
            return [fast_sim_dep, raw_index_dep]
        
        def slcio_files(self, raw_index_task: 'AbstractIndex'):        
            raw_index_4f = raw_index_task.input()[1]        
            samples_4f = np.load(raw_index_4f[1].path)
            
            semileptonic_processes = list(filter(lambda p: '_sl0' in p, np.unique(samples_4f['process']).tolist()))

            input_files = []
            for proc in semileptonic_processes:
                input_files += samples_4f['location'][samples_4f['process'] == proc].tolist()
                
            input_files.sort()

            return input_files
        
        marlin_globals = {  }
        marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

        sgv_executable = sgv_executable_cfg
        sgv_steering_file_src = sgv_steering_file_src_cfg

    class Config_550_4f(AnalysisConfiguration):
        tag = f'550-4f-{suffix}'
        
        def sgv_inputs(self, fast_sim_task):
            input_files = glob_exp('$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/*.slcio')
            input_files = list(filter(lambda path: 'pilot.slcio' not in path and '.0.slcio' not in path, input_files))

            # files which are valid LCIO files, but cannot be processed by SGV
            invalid_files = list(map(osp.expandvars, [
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.192.singleZee.leptonic.eL_pR.a160.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.195.singleZee.leptonic.eL_pR.a160.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.121.singleZee.leptonic.eL_pR.a107.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.227.singleZee.leptonic.eL_pR.a213.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pR.I501055.6.singleZee.leptonic.eR_pR.a0.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.125.singleZee.leptonic.eL_pR.a107.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.46.singleZee.leptonic.eL_pR.a0.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.148.singleZee.leptonic.eL_pR.a107.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.351.singleZee.leptonic.eL_pR.a319.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.214.singleZee.leptonic.eL_pR.a213.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pL.I501056.184.singleZee.leptonic.eR_pL.a168.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pR.I501055.16.singleZee.leptonic.eR_pR.a0.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.216.singleZee.leptonic.eL_pR.a213.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pL.I501056.273.singleZee.leptonic.eR_pL.a267.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.182.singleZee.leptonic.eL_pR.a160.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.328.singleZee.leptonic.eL_pR.a319.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.171.singleZee.leptonic.eL_pR.a160.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pL.I501056.351.singleZee.leptonic.eR_pL.a333.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.358.singleZee.leptonic.eL_pR.a319.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.306.singleZee.leptonic.eL_pR.a266.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pR.I501055.46.singleZee.leptonic.eR_pR.a36.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pL.I501056.217.singleZee.leptonic.eR_pL.a201.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.53.singleZee.leptonic.eL_pR.a0.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.226.singleZee.leptonic.eL_pR.a213.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pL.I501053.59.singleZee.leptonic.eL_pL.a36.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.264.singleZee.leptonic.eL_pR.a213.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.154.singleZee.leptonic.eL_pR.a107.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pL.I501056.228.singleZee.leptonic.eR_pL.a201.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.163.singleZee.leptonic.eL_pR.a160.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.110.singleZee.leptonic.eL_pR.a107.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.338.singleZee.leptonic.eL_pR.a319.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pL.I501053.50.singleZee.leptonic.eL_pL.a36.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pL.I501056.278.singleZee.leptonic.eR_pL.a267.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pL.I501056.254.singleZee.leptonic.eR_pL.a234.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.217.singleZee.leptonic.eL_pR.a213.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.326.singleZee.leptonic.eL_pR.a319.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pL.I501056.28.singleZee.leptonic.eR_pL.a0.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pL.I501056.223.singleZee.leptonic.eR_pL.a201.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.133.singleZee.leptonic.eL_pR.a107.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pL.I501056.310.singleZee.leptonic.eR_pL.a300.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pL.I501056.294.singleZee.leptonic.eR_pL.a267.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pL.I501056.96.singleZee.leptonic.eR_pL.a68.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pL.I501056.211.singleZee.leptonic.eR_pL.a201.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eR.pL.I501056.17.singleZee.leptonic.eR_pL.a0.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_l.Gwhizard-3_1_4.eL.pR.I501054.280.singleZee.leptonic.eL_pR.a266.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_szeorsw_l.Gwhizard-3_1_4.eL.pR.I501066.0.singleZsingleWMix.leptonic.eL_pR.a0.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_szeorsw_l.Gwhizard-3_1_4.eL.pR.I501066.17.singleZsingleWMix.leptonic.eL_pR.a9.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_szeorsw_l.Gwhizard-3_1_4.eL.pR.I501066.51.singleZsingleWMix.leptonic.eL_pR.a45.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_szeorsw_l.Gwhizard-3_1_4.eL.pR.I501066.20.singleZsingleWMix.leptonic.eL_pR.a18.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_szeorsw_l.Gwhizard-3_1_4.eR.pL.I501068.1.singleZsingleWMix.leptonic.eR_pL.a0.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sw_l.Gwhizard-3_1_4.eL.pR.I501058.9.singleW.leptonic.eL_pR.a0.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sw_l.Gwhizard-3_1_4.eL.pR.I501058.72.singleW.leptonic.eL_pR.a65.slcio',
                '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_sl.Gwhizard-3_1_4.eL.pR.I501042.83.singleZee.semileptonic.eL_pR.a52.slcio']))

            input_files = list(set(input_files) - set(invalid_files))
            input_files.sort()
            
            input_options = [{
                'global_steering.MAXEV': 999999,
                'global_generation_steering.CMS_ENE': 550,
                'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
                'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
                **sgv_options
            }] * len(input_files)
            
            return input_files, input_options
        
        marlin_globals = {  }
        marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

        sgv_executable = sgv_executable_cfg
        sgv_steering_file_src = sgv_steering_file_src_cfg

    class Config_550_tthz(AnalysisConfiguration):
        tag = f'550-tthz-{suffix}'
        
        def sgv_inputs(self, fast_sim_task):
            input_files = glob_exp('$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/8f/*.slcio')
            input_files.sort()
            
            input_options = [{
                'global_steering.MAXEV': 999999,
                'global_generation_steering.CMS_ENE': 550,
                'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
                'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
                **sgv_options
            }] * len(input_files)
            
            return input_files, input_options
        
        marlin_globals = {  }
        marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

        sgv_executable = sgv_executable_cfg
        sgv_steering_file_src = sgv_steering_file_src_cfg

    class Config_550_2l(AnalysisConfiguration):
        tag = f'550-2l-{suffix}'
        
        def sgv_inputs(self, fast_sim_task):
            input_files = glob_exp('$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/2f/*P2f_z_l*.slcio')
            input_files.sort()
            
            input_options = [{
                'global_steering.MAXEV': 999999,
                'global_generation_steering.CMS_ENE': 550,
                'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
                'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
                **sgv_options
            }] * len(input_files)
            
            return input_files, input_options
        
        marlin_globals = {  }
        marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

        sgv_executable = sgv_executable_cfg
        sgv_steering_file_src = sgv_steering_file_src_cfg

    class Config_550_4flh(AnalysisConfiguration):
        """Configuration for all 4f fully leptonic and fully hadronic samples

        Args:
            AnalysisConfiguration (_type_): _description_

        Returns:
            _type_: _description_
        """

        tag = f'550-4flh-{suffix}'
        
        def raw_index_requires(self, raw_index_task: 'AbstractIndex'):
            # use the output of 550-4f-{suffix} as input
            
            from workflows.analysis.tasks_sim import FastSimSGV
            from analysis.tasks import RawIndex
            
            fast_sim_dep = FastSimSGV.req(raw_index_task, tag=f'550-4f-{suffix}')
            raw_index_dep = RawIndex.req(raw_index_task, tag=f'550-4f-{suffix}') 
                        
            return [fast_sim_dep, raw_index_dep]
        
        def slcio_files(self, raw_index_task: 'AbstractIndex'):
            raw_index_4f = raw_index_task.input()[1]
            samples_4f = np.load(raw_index_4f[1].path)
            
            semileptonic_processes = list(filter(lambda p: '_h' in p or '_l' in p, np.unique(samples_4f['process']).tolist()))

            input_files = []
            for proc in semileptonic_processes:
                input_files += samples_4f['location'][samples_4f['process'] == proc].tolist()
                
            input_files.sort()

            return input_files
        
        marlin_globals = {  }
        marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

    class Config_550_2l4q(AnalysisConfiguration):
        tag = f'550-2l4q-{suffix}'
        
        def sgv_inputs(self, fast_sim_task):
            process_mask_2l4q = [
                '6f_xxvlyx',
                '6f_yyxyev',
                '6f_llxxxx',
                '6f_llxyyx',
                '6f_eexxxx',
                '6f_yyveyx',
                '6f_vvxxxx',
                '6f_xxxylv',
                '6f_yyvlyx',
                '6f_eeyyyy',
                '6f_vvyyyy',
                '6f_xxxyev',
                '6f_yyxylv',
                '6f_xxveyx',
                '6f_eexyyx',
                '6f_vvxyyx',
                '6f_llyyyy']
            
            input_files:list[str] = []
            for process_mask in process_mask_2l4q:
                input_files += glob_exp(f'$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/6f/*{process_mask}*.slcio')
            
            input_options = [{
                'global_steering.MAXEV': 999999,
                'global_generation_steering.CMS_ENE': 550,
                'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
                'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
                **sgv_options
            }] * len(input_files)
            
            return input_files, input_options
        
        marlin_globals = {  }
        marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

        sgv_executable = sgv_executable_cfg
        sgv_steering_file_src = sgv_steering_file_src_cfg
    
    zhh_configs.add(Config_550_llhh())
    zhh_configs.add(Config_550_vvhh())
    zhh_configs.add(Config_550_qqhh())
    zhh_configs.add(Config_550_6q())
    zhh_configs.add(Config_550_4fsl())
    zhh_configs.add(Config_550_4f())
    zhh_configs.add(Config_550_tthz())
    zhh_configs.add(Config_550_2l())
    zhh_configs.add(Config_550_4flh())
    zhh_configs.add(Config_550_2l4q())