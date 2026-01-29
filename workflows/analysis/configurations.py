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

from .configs.conf_550_fast_perf import *
from .configs.conf_550_fast_pfl import *
from .configs.conf_550_full import *

#print('Configs:')
#for key, val in zhh_configs.definitions.items():
#    print(key)