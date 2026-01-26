# This file contains task parameters to define so-called tags
# They are a required argument when calling law via
#     law run <task> --tag=<tag>
# These allow to re-use many of the task definitions by only
# varying input files and other task inputs or dynamically
# injecting task dependencies.
# Tasks of different tags may also depend on each other. See
# the 550-4fsl-fast-perf config which depends on 550-4f-fast-
# perf. For example, the 4f config defines the input for the
# FastSimSGV to be all available 4f samples, the 4fsl then
# depends on 4f and only uses the semileptonic samples.

from analysis.framework import AnalysisConfiguration, zhh_configs
from typing import TYPE_CHECKING
from os import environ, path as osp
import numpy as np
from zhh import glob_exp

if TYPE_CHECKING:
    from analysis.tasks import RawIndex, AbstractIndex
    
# only configurations
if False:
    # example how to use Whizard before SGV
    class Config_550_llbb_fast_perf(AnalysisConfiguration):
        tag = '550-llbb-fast-perf'
        
        whizard_options = [
            { 'process_name': 'eebb_sl0', 'process_definition': '', 'template_dir': '$REPO_ROOT/workflows/resources/whizard_template', 'sindarin_file': 'whizard.base.sin' },
            { 'process_name': 'llbb_sl0', 'process_definition': '', 'template_dir': '$REPO_ROOT/workflows/resources/whizard_template', 'sindarin_file': 'whizard.base.sin' }
        ]
        
        def sgv_inputs(self, fast_sim_task):
            from analysis.tasks_reco import FastSimSGV
            assert(isinstance(fast_sim_task, FastSimSGV))
            
            sgv_inputs = fast_sim_task.input()
            assert('whizard_event_generation' in sgv_inputs)
            
            whiz_outputs = sgv_inputs['whizard_event_generation']['collection']
            
            input_files:list[str] = []
            for i in range(len(whiz_outputs)):
                input_files.append(whiz_outputs[i][0].path)
            
            input_options = [{
                'global_steering.MAXEV': 999999,
                'global_generation_steering.CMS_ENE': 550,
                'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
                'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
                'analysis_steering.CALO_TREATMENT': 'PERF'
            }] * len(input_files)
            
            return input_files, input_options
        
        marlin_globals = {  }
        marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

    class Config_550_llbb_fast_pfl(AnalysisConfiguration):
        tag = '550-llbb-fast-pfl'
        
        whizard_options = [
            { 'process_name': 'eebb_sl0', 'process_definition': '', 'template_dir': '$REPO_ROOT/workflows/resources/whizard_template', 'sindarin_file': 'whizard.base.sin' },
            { 'process_name': 'llbb_sl0', 'process_definition': '', 'template_dir': '$REPO_ROOT/workflows/resources/whizard_template', 'sindarin_file': 'whizard.base.sin' }
        ]
        
        def sgv_inputs(self, fast_sim_task):
            from analysis.tasks_reco import FastSimSGV
            assert(isinstance(fast_sim_task, FastSimSGV))
            
            sgv_inputs = fast_sim_task.input()
            assert('whizard_event_generation' in sgv_inputs)
            
            whiz_outputs = sgv_inputs['whizard_event_generation']['collection']
            
            input_files:list[str] = []
            for i in range(len(whiz_outputs)):
                input_files.append(whiz_outputs[i][0].path)
            
            input_options = [{
                'global_steering.MAXEV': 999999,
                'global_generation_steering.CMS_ENE': 550,
                'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
                'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
                'analysis_steering.CALO_TREATMENT': 'PFL '
            }] * len(input_files)
            
            return input_files, input_options
        
        marlin_globals = {  }
        marlin_constants = { 'CMSEnergy': 550 }
        
# example how to use Whizard before SGV
class Config_500_zh_tau_fast_perf(AnalysisConfiguration):
    tag = '500-zh-tau-fast-perf'
    
    whizard_options = [
        { 'process_name': 'zh_e3e3nunu',
         'process_definition': '',
         'template_dir': '$REPO_ROOT/workflows/resources/whizard_template',
         'sindarin_file': 'whizard.base500.sin',
         'iters_per_polarization': { 'eL.pL': 10, 'eL.pR': 10, 'eR.pL': 10, 'eR.pR': 10 }},
        { 'process_name': 'zh_ddh',
         'process_definition': '',
         'template_dir': '$REPO_ROOT/workflows/resources/whizard_template',
         'sindarin_file': 'whizard.base500.sin',
         'iters_per_polarization': { 'eL.pL': 10, 'eL.pR': 10, 'eR.pL': 10, 'eR.pR': 10 } },
        { 'process_name': 'zh_uuh',
         'process_definition': '',
         'template_dir': '$REPO_ROOT/workflows/resources/whizard_template',
         'sindarin_file': 'whizard.base500.sin',
         'iters_per_polarization': { 'eL.pL': 10, 'eL.pR': 10, 'eR.pL': 10, 'eR.pR': 10 } },
        { 'process_name': 'zh_ssh',
         'process_definition': '',
         'template_dir': '$REPO_ROOT/workflows/resources/whizard_template',
         'sindarin_file': 'whizard.base500.sin',
         'iters_per_polarization': { 'eL.pL': 10, 'eL.pR': 10, 'eR.pL': 10, 'eR.pR': 10 } },
        { 'process_name': 'zh_cch',
         'process_definition': '',
         'template_dir': '$REPO_ROOT/workflows/resources/whizard_template',
         'sindarin_file': 'whizard.base500.sin',
         'iters_per_polarization': { 'eL.pL': 10, 'eL.pR': 10, 'eR.pL': 10, 'eR.pR': 10 } },
        { 'process_name': 'zh_bbh',
         'process_definition': '',
         'template_dir': '$REPO_ROOT/workflows/resources/whizard_template',
         'sindarin_file': 'whizard.base500.sin',
         'iters_per_polarization': { 'eL.pL': 10, 'eL.pR': 10, 'eR.pL': 10, 'eR.pR': 10 } }
    ]
    
    def sgv_inputs(self, fast_sim_task):
        from analysis.tasks_reco import FastSimSGV
        assert(isinstance(fast_sim_task, FastSimSGV))
        
        sgv_inputs = fast_sim_task.input()
        assert('whizard_event_generation' in sgv_inputs)
        
        whiz_outputs = sgv_inputs['whizard_event_generation']['collection']
        
        input_files:list[str] = []
        for i in range(len(whiz_outputs)):
            input_files.append(whiz_outputs[i][0].path)
        
        input_options = [{
            'global_steering.MAXEV': 999999,
            'global_generation_steering.CMS_ENE': 500,
            'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
            'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
            'analysis_steering.CALO_TREATMENT': 'PERF'
        }] * len(input_files)
        
        return input_files, input_options
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 500, 'errorflowconfusion': 'False' }

class Config_500_zh10_tau_fast_perf(AnalysisConfiguration):
    tag = '500-zh10-tau-fast-perf'
    
    whizard_options = [
        { 'process_name': 'zh_e3e3nunu',
         'process_definition': '',
         'template_dir': '$REPO_ROOT/workflows/resources/whizard_template',
         'sindarin_file': 'whizard.base500.sin',
         'iters_per_polarization': { 'eL.pR': 200, 'eR.pL': 200 }},
        { 'process_name': 'zh_ddh',
         'process_definition': '',
         'template_dir': '$REPO_ROOT/workflows/resources/whizard_template',
         'sindarin_file': 'whizard.base500.sin',
         'iters_per_polarization': { 'eL.pR': 200, 'eR.pL': 200 } },
        { 'process_name': 'zh_uuh',
         'process_definition': '',
         'template_dir': '$REPO_ROOT/workflows/resources/whizard_template',
         'sindarin_file': 'whizard.base500.sin',
         'iters_per_polarization': { 'eL.pR': 200, 'eR.pL': 200 } },
        { 'process_name': 'zh_ssh',
         'process_definition': '',
         'template_dir': '$REPO_ROOT/workflows/resources/whizard_template',
         'sindarin_file': 'whizard.base500.sin',
         'iters_per_polarization': { 'eL.pR': 200, 'eR.pL': 200 } },
        { 'process_name': 'zh_cch',
         'process_definition': '',
         'template_dir': '$REPO_ROOT/workflows/resources/whizard_template',
         'sindarin_file': 'whizard.base500.sin',
         'iters_per_polarization': { 'eL.pR': 200, 'eR.pL': 200 } },
        { 'process_name': 'zh_bbh',
         'process_definition': '',
         'template_dir': '$REPO_ROOT/workflows/resources/whizard_template',
         'sindarin_file': 'whizard.base500.sin',
         'iters_per_polarization': { 'eL.pR': 200, 'eR.pL': 200 } }
    ]
    
    def sgv_inputs(self, fast_sim_task):
        from analysis.tasks_reco import FastSimSGV
        assert(isinstance(fast_sim_task, FastSimSGV))
        
        sgv_inputs = fast_sim_task.input()
        assert('whizard_event_generation' in sgv_inputs)
        
        whiz_outputs = sgv_inputs['whizard_event_generation']['collection']
        
        input_files:list[str] = []
        for i in range(len(whiz_outputs)):
            input_files.append(whiz_outputs[i][0].path)
        
        input_options = [{
            'global_steering.MAXEV': 999999,
            'global_generation_steering.CMS_ENE': 500,
            'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
            'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
            'analysis_steering.CALO_TREATMENT': 'PERF'
        }] * len(input_files)
        
        return input_files, input_options
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 500, 'errorflowconfusion': 'False' }

#################################
# FULL SIM                      #
#################################

class Config_550_llhh_full(AnalysisConfiguration):
    tag = '550-llhh-full'
    
    def slcio_files(self, raw_index_task: 'RawIndex'):
        result = []
        base = '$ILC_PROD_PATH/mc-2020/ild/dst-merged/550-Test/hh/ILD_l5_o1_v02/v02-02-03'
        
        for mask in ['Pe1e1', 'Pe2e2', 'Pe3e3']:
            result += glob_exp(f'{base}/**/*{mask}*.slcio', recursive=True)  
        
        return result
    
    def __init__(self):
        super().__init__()
        
        self.marlin_constants = {
            'CMSEnergy': 550,
            'LCFIPlusML_ONNX': f'{environ["REPO_ROOT"]}/dependencies/LCFIPlusConfig/onnx/ilc_nnqq_neutrals/ilc_nnqq_neutrals.onnx',
            'LCFIPlusML_JSON': f'{environ["REPO_ROOT"]}/dependencies/LCFIPlusConfig/onnx/ilc_nnqq_neutrals/preprocess.json'
        }

class Config_250_ftag_full(AnalysisConfiguration):
    tag = '250-ftag-full'
    
    task_kwargs:dict[str, dict] = {
        'MarlinBaseJob': {
            'debug_n_files_to_process': 0, # process all files
            'n_events_max': 0, # process all events
            'steering_file': '$REPO_ROOT/scripts/dev_flavortag_compare.xml',
            'check_output_root_ttrees': [],
            'check_output_files_exist': [],
            'output_file': 'FT_compare_AIDA.root'
    }}
    
    def slcio_files(self, raw_index_task: 'RawIndex'):
        result = []
        base = '$ILC_PROD_PATH/mc-2020/ild/dst-merged/250-SetA/flavortag/ILD_l5_o1_v02/v02-02'
        
        for mask in ['zz_dddd', 'zz_uuuu', 'zz_ssss', 'zz_cccc', 'zz_bbbb']:
            result += glob_exp(f'{base}/**/*{mask}*.slcio', recursive=True)

        result.sort()
        
        return result
    
    # with these settings, AnalysisRuntime actually gives the final result already!
    def __init__(self):
        super().__init__()
        
        self.marlin_constants = {
        'CMSEnergy': 250,
        'LCFIPlusML_ONNX': f'{environ["REPO_ROOT"]}/dependencies/LCFIPlusConfig/onnx/ilc_nnqq_neutrals/ilc_nnqq_neutrals.onnx',
        'LCFIPlusML_JSON': f'{environ["REPO_ROOT"]}/dependencies/LCFIPlusConfig/onnx/ilc_nnqq_neutrals/preprocess.json'
    }
        
class Config_250_ftag_fast_perf(AnalysisConfiguration):
    tag = '250-ftag-fast-perf'
    
    task_kwargs:dict[str, dict] = {
        'MarlinBaseJob': {
            'debug_n_files_to_process': 0, # process all files
            'n_events_max': 0, # process all events
            'steering_file': '$REPO_ROOT/scripts/dev_flavortag_compare.xml',
            'check_output_root_ttrees': [],
            'check_output_files_exist': [],
            'output_file': 'FT_compare_AIDA.root'
    }}
    
    def sgv_inputs(self, fast_sim_task):
        input_files = []
        base = '$ILC_PROD_PATH/mc-2020/generated/250-SetA/flavortag'
        
        for mask in ['zz_dddd', 'zz_uuuu', 'zz_ssss', 'zz_cccc', 'zz_bbbb']:
            input_files += glob_exp(f'{base}/*{mask}*.slcio')
        
        input_files.sort()
        
        input_options = [{
            'global_steering.MAXEV': 999999,
            'global_generation_steering.CMS_ENE': 250,
            'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
            'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
            'analysis_steering.CALO_TREATMENT': 'PERF'
        }] * len(input_files)
        
        return input_files, input_options
    
    # with these settings, AnalysisRuntime actually gives the final result already!
    def __init__(self):
        super().__init__()
        
        self.marlin_constants = {
        'CMSEnergy': 250
        }

#################################
# FAST SIM SGV PERF + PFL       #
#################################

class Config_550_llhh_fast_perf(AnalysisConfiguration):
    tag = '550-llhh-fast-perf'
    
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
            'analysis_steering.CALO_TREATMENT': 'PERF'
        }] * len(input_files)
        
        return input_files, input_options
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

class Config_550_vvhh_fast_perf(AnalysisConfiguration):
    tag = '550-vvhh-fast-perf'
    
    def sgv_inputs(self, fast_sim_task):
        input_files:list[str] = sum(map(glob_exp, [
            '$ILC_PROD_PATH/mc-2020/generated/550-Test/hh/*Pn1n1*.slcio',
            '$ILC_PROD_PATH/mc-2020/generated/550-Test/hh/*Pn23n23*.slcio',
        ]), [])
        
        # filter out invalid files
        #invalid_files = list(map(osp.expandvars, []))
        #input_files = list(set(input_files) - set(invalid_files))
        
        input_files.sort()
        
        input_options = [{
            'global_steering.MAXEV': 999999,
            'global_generation_steering.CMS_ENE': 550,
            'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
            'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
            'analysis_steering.CALO_TREATMENT': 'PERF'
        }] * len(input_files)
        
        return input_files, input_options
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

class Config_550_llhh_fast_pfl(AnalysisConfiguration):
    tag = '550-llhh-fast-pfl'
    
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
            'analysis_steering.CALO_TREATMENT': 'PFL '
        }] * len(input_files)
        
        return input_files, input_options
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550 }

class Config_550_4fh_fast_perf(AnalysisConfiguration):
    tag = '550-4fh-fast-perf'
    
    def raw_index_requires(self, raw_index_task: 'AbstractIndex'):
        # use the output of 550-4f-fast-perf as input
        
        from analysis.tasks_reco import FastSimSGV
        from analysis.tasks import RawIndex
        
        fast_sim_dep = FastSimSGV.req(raw_index_task, tag='550-4f-fast-perf')
        raw_index_dep = RawIndex.req(raw_index_task, tag='550-4f-fast-perf') 
                     
        return [fast_sim_dep, raw_index_dep]
    
    def slcio_files(self, raw_index_task: 'AbstractIndex'):
        raw_index_4f = raw_index_task.input()[1]
        samples_4f = np.load(raw_index_4f[1].path)
        
        semileptonic_processes = list(filter(lambda p: '_h' in p, np.unique(samples_4f['process']).tolist()))

        input_files = []
        for proc in semileptonic_processes:
            input_files += samples_4f['location'][samples_4f['process'] == proc].tolist()
            
        input_files.sort()

        return input_files
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

class Config_550_4fsl_fast_perf(AnalysisConfiguration):
    tag = '550-4fsl-fast-perf'
    
    custom_statistics = [
        (1., 'zz_sl0')
    ]
    
    def raw_index_requires(self, raw_index_task: 'AbstractIndex'):
        # use the output of 550-4f-fast-perf as input
        
        from analysis.tasks_reco import FastSimSGV
        from analysis.tasks import RawIndex
        
        fast_sim_dep = FastSimSGV.req(raw_index_task, tag='550-4f-fast-perf')
        raw_index_dep = RawIndex.req(raw_index_task, tag='550-4f-fast-perf') 
                     
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

class Config_550_4f_fast_perf(AnalysisConfiguration):
    tag = '550-4f-fast-perf'
    
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
            '$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/4f/Nov2025/E550-TDR_ws.P4f_sze_sl.Gwhizard-3_1_4.eL.pR.I501042.83.singleZee.semileptonic.eL_pR.a52.slcio'
        ]))

        input_files = list(set(input_files) - set(invalid_files))
        input_files.sort()
        
        input_options = [{
            'global_steering.MAXEV': 999999,
            'global_generation_steering.CMS_ENE': 550,
            'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
            'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
            'analysis_steering.CALO_TREATMENT': 'PERF'
        }] * len(input_files)
        
        return input_files, input_options
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

class Config_550_tthz_fast_perf(AnalysisConfiguration):
    tag = '550-tthz-fast-perf'
    
    def sgv_inputs(self, fast_sim_task):
        input_files = glob_exp('$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/8f/*.slcio')
        input_files.sort()
        
        input_options = [{
            'global_steering.MAXEV': 999999,
            'global_generation_steering.CMS_ENE': 550,
            'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
            'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
            'analysis_steering.CALO_TREATMENT': 'PERF'
        }] * len(input_files)
        
        return input_files, input_options
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

class Config_550_2l_fast_perf(AnalysisConfiguration):
    tag = '550-2l-fast-perf'
    
    def sgv_inputs(self, fast_sim_task):
        input_files = glob_exp('$ILC_PROD_PATH/mc-2025/generated/550-TDR_ws/2f/*P2f_z_l*.slcio')
        input_files.sort()
        
        input_options = [{
            'global_steering.MAXEV': 999999,
            'global_generation_steering.CMS_ENE': 550,
            'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
            'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
            'analysis_steering.CALO_TREATMENT': 'PERF'
        }] * len(input_files)
        
        return input_files, input_options
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

class Config_550_2l4q_fast_perf(AnalysisConfiguration):
    tag = '550-2l4q-fast-perf'
    
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
            'analysis_steering.CALO_TREATMENT': 'PERF'
        }] * len(input_files)
        
        return input_files, input_options
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

class Config_550_2l4q_fast_pfl(AnalysisConfiguration):
    tag = '550-2l4q-fast-pfl'
    
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
            'analysis_steering.CALO_TREATMENT': 'PFL '
        }] * len(input_files)
        
        return input_files, input_options
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550 }

class Config_550_6q_fast_perf(AnalysisConfiguration):
    tag = '550-6q-fast-perf'
    
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
        #invalid_files = list(map(osp.expandvars, []))
        #input_files = list(set(input_files) - set(invalid_files))
        
        input_files.sort()
        
        input_options = [{
            'global_steering.MAXEV': 999999,
            'global_generation_steering.CMS_ENE': 550,
            'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
            'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
            'analysis_steering.CALO_TREATMENT': 'PERF'
        }] * len(input_files)
        
        return input_files, input_options
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }
    
class Config_550_6q_fast_pfl(AnalysisConfiguration):
    tag = '550-6q-fast-pfl'
    
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
        #invalid_files = list(map(osp.expandvars, []))
        #input_files = list(set(input_files) - set(invalid_files))

        input_files.sort()
        
        input_options = [{
            'global_steering.MAXEV': 999999,
            'global_generation_steering.CMS_ENE': 550,
            'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
            'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
            'analysis_steering.CALO_TREATMENT': 'PFL '
        }] * len(input_files)
        
        return input_files, input_options
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550 }

class Config_550_bbbb_fast_perf(AnalysisConfiguration):
    tag = '550-bbbb-fast-perf'
    
    whizard_options = [
        { 'process_name': 'bbbb_sl0', 'process_definition': '', 'template_dir': '$REPO_ROOT/workflows/resources/whizard_template', 'sindarin_file': 'whizard.base.sin', 'iters_per_polarization': {} }
    ]
    
    def sgv_inputs(self, fast_sim_task):
        from analysis.tasks_reco import FastSimSGV
        assert(isinstance(fast_sim_task, FastSimSGV))
        
        sgv_inputs = fast_sim_task.input()
        assert('whizard_event_generation' in sgv_inputs)
        
        whiz_outputs = sgv_inputs['whizard_event_generation']['collection']
        
        input_files:list[str] = []
        for i in range(len(whiz_outputs)):
            input_files.append(whiz_outputs[i][0].path)
        
        input_options = [{
            'global_steering.MAXEV': 999999,
            'global_generation_steering.CMS_ENE': 550,
            'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
            'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
            'analysis_steering.CALO_TREATMENT': 'PERF'
        }] * len(input_files)
        
        return input_files, input_options
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550, 'errorflowconfusion': 'False' }

# legacy
#zhh_configs.add(Config_500_all_full())
zhh_configs.add(Config_500_zh_tau_fast_perf())
zhh_configs.add(Config_500_zh10_tau_fast_perf())

# FULL SIM
# ftag
zhh_configs.add(Config_250_ftag_full())

# llHH
zhh_configs.add(Config_550_llhh_full())

# SGV PERF
# llHH
zhh_configs.add(Config_550_llhh_fast_perf())
zhh_configs.add(Config_550_4fsl_fast_perf())
zhh_configs.add(Config_550_4fh_fast_perf())
zhh_configs.add(Config_550_2l4q_fast_perf())
zhh_configs.add(Config_550_bbbb_fast_perf())
zhh_configs.add(Config_550_tthz_fast_perf())
zhh_configs.add(Config_550_2l_fast_perf())
zhh_configs.add(Config_250_ftag_fast_perf())

# qqHH
zhh_configs.add(Config_550_6q_fast_perf())

# vvHH
zhh_configs.add(Config_550_vvhh_fast_perf())

# SGV PFL

# llHH
zhh_configs.add(Config_550_llhh_fast_pfl())
#zhh_configs.add(Config_550_llbb_fast_pfl())
zhh_configs.add(Config_550_2l4q_fast_pfl())
zhh_configs.add(Config_550_6q_fast_pfl())
zhh_configs.add(Config_550_4f_fast_perf())