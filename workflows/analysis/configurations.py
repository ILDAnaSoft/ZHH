from analysis.framework import AnalysisConfiguration, AnalysisConfigurationRegistry, zhh_configs
from zhh import get_raw_files
from glob import glob
from typing import TYPE_CHECKING
from .utils.types import SGVOptions, WhizardOption
from law import Task

if TYPE_CHECKING:
    from analysis.tasks import RawIndex

# Define the configurations for the analysis
class Config_500_all_full(AnalysisConfiguration):
    tag = '500-all-full'
    
    def slcio_files(self, raw_index_task: 'RawIndex'):
        return get_raw_files()
    
    statistics = 1.
    custom_statistics = [
        [100, ["e1e1hh", "e2e2hh", "e3e3hh", "e1e1qqh", "e2e2qqh", "e3e3qqh",
        "n1n1hh", "n23n23hh", "n1n1qqh", "n23n23qqh",
        "qqhh", "qqqqh"], "expected"]
    ]
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 500 }

class Config_550_hh_fast(AnalysisConfiguration):
    tag = '550-hh-fast'
    
    def sgv_inputs(self, fast_sim_task):
        input_files = glob('/pnfs/desy.de/ilc/prod/ilc/mc-2020/generated/550-Test/hh/*.slcio')
        input_files.sort()
        input_options = [{
            'analysis_steering.CALO_TREATMENT': 'PFL '
        }] * len(input_files)
        
        return input_files, input_options
    
    statistics = 1.
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550 }

class Config_550_hh_fast_perf(AnalysisConfiguration):
    tag = '550-hh-fast-perf'
    
    def sgv_inputs(self, fast_sim_task):
        input_files:list[str] = []
        input_options:list[dict] = []
        
        for cms_energy, sgv_input_format, source_dir, file_ending in [
            (550, 'LCIO', '/pnfs/desy.de/ilc/prod/ilc/mc-2020/generated/550-Test/hh', 'slcio'),
        ]:
            sgv_options:SGVOptions = {
                'global_steering.MAXEV': 999999,
                'global_generation_steering.CMS_ENE': cms_energy,
                'external_read_generation_steering.GENERATOR_INPUT_TYPE': sgv_input_format,
                'external_read_generation_steering.INPUT_FILENAMES': f'input.{file_ending}',
                'analysis_steering.CALO_TREATMENT': 'PERF'
            }
            files = glob(f'{source_dir}/*.{file_ending}')
            files.sort()
            
            for file in files:
                input_files.append(file)
                input_options.append(sgv_options)
        
        return input_files, input_options
    
    statistics = 1.
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550 }

class Config_550_hh_full(AnalysisConfiguration):
    tag = '550-hh-full'
    
    def slcio_files(self, raw_index_task: 'RawIndex'):
        return glob(f'/pnfs/desy.de/ilc/prod/ilc/mc-2020/ild/dst-merged/550-Test/hh/ILD_l5_o1_v02/v02-02-03/**/*.slcio', recursive=True)  
    
    # Use only e2e2hh (because the generator samples are only available for e2e2hh)
    statistics = 0.
    custom_statistics = [
        [1., ["e2e2hh"], "total"]
    ]
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550 }
    
class Config_550_llhh_fast_perf(AnalysisConfiguration):
    tag = '550-llhh-fast-perf'
    
    def sgv_inputs(self):
        input_files:list[str] = []
        input_options:list[dict] = []
        
        for cms_energy, sgv_input_format, source_glob, file_ending in [
            (550, 'LCIO', '/pnfs/desy.de/ilc/prod/ilc/mc-2020/generated/550-Test/6f-test/*Pe1e1*.slcio', 'slcio'),
            (550, 'LCIO', '/pnfs/desy.de/ilc/prod/ilc/mc-2020/generated/550-Test/6f-test/*Pe2e2*.slcio', 'slcio'),
            (550, 'LCIO', '/pnfs/desy.de/ilc/prod/ilc/mc-2020/generated/550-Test/6f-test/*Pe3e3*.slcio', 'slcio'),
        ]:
            sgv_options:SGVOptions = {
                'global_steering.MAXEV': 999999,
                'global_generation_steering.CMS_ENE': cms_energy,
                'external_read_generation_steering.GENERATOR_INPUT_TYPE': sgv_input_format,
                'external_read_generation_steering.INPUT_FILENAMES': f'input.{file_ending}',
                'analysis_steering.CALO_TREATMENT': 'PERF'
            }
            files = glob(source_glob)
            files.sort()
            
            for file in files:
                input_files.append(file)
                input_options.append(sgv_options)
        
        return input_files, input_options
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550 }
   
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
    marlin_constants = { 'CMSEnergy': 550 }

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
            input_files += glob(f'/pnfs/desy.de/ilc/prod/ilc/mc-2025/generated/550-TDR_ws/6f/*{process_mask}*.slcio')
        
        input_options = [{
            'global_steering.MAXEV': 999999,
            'global_generation_steering.CMS_ENE': 550,
            'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
            'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
            'analysis_steering.CALO_TREATMENT': 'PERF'
        }] * len(input_files)
        
        return input_files, input_options
    
    marlin_globals = {  }
    marlin_constants = { 'CMSEnergy': 550 }

# Add them to the registry
zhh_configs.add(Config_500_all_full())
zhh_configs.add(Config_550_hh_fast())
zhh_configs.add(Config_550_hh_full())
zhh_configs.add(Config_550_hh_fast_perf())

# llHH
zhh_configs.add(Config_550_llhh_fast_perf())
zhh_configs.add(Config_550_llbb_fast_perf())
zhh_configs.add(Config_550_2l4q_fast_perf())