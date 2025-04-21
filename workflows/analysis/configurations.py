from analysis.framework import AnalysisConfiguration, AnalysisConfigurationRegistry, zhh_configs
from zhh import get_raw_files
from glob import glob
from typing import TYPE_CHECKING
from .utils.types import SGVOptions, WhizardOption

if TYPE_CHECKING:
    from analysis.tasks import RawIndex
    from analysis.tasks_reco import FastSimSGV

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
    
    def sgv_inputs(self):
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
    
    def sgv_inputs(self):
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
    
    def sgv_inputs(self, fast_sim_task: 'FastSimSGV'):
        input_files:list[str] = []
        input_options:list[dict] = []
        
        print(fast_sim_task.input())
        raise Exception('Not implemented')
        
        for cms_energy, sgv_input_format, file_ending in [
            (550, 'LCIO', 'slcio'),
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

# Add them to the registry
zhh_configs.add(Config_500_all_full())
zhh_configs.add(Config_550_hh_fast())
zhh_configs.add(Config_550_hh_full())
zhh_configs.add(Config_550_hh_fast_perf())

# llHH
zhh_configs.add(Config_550_llhh_fast_perf())
zhh_configs.add(Config_550_llbb_fast_perf())
#zhh_configs.add(Config_550_2l4q_fast_perf())