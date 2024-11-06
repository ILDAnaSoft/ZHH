from analysis.framework import AnalysisConfiguration, AnalysisConfigurationRegistry, zhh_configs
from zhh import get_raw_files
from glob import glob
from typing import TYPE_CHECKING
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
    
    # Attach SGV as a requirement for the RawIndex task and define the input files
    def index_requires(self, raw_index_task: 'RawIndex')->list:
        from analysis.tasks_reco import FastSimSGV
        
        fast_sim_task = FastSimSGV.req(raw_index_task)
        fast_sim_task.sgv_input_files = glob('/pnfs/desy.de/ilc/prod/ilc/mc-2020/generated/550-Test/hh/*.slcio')
        
        return [fast_sim_task]
    
    # Retrieve the paths to the output files of the SGV runs
    def slcio_files(self, raw_index_task: 'RawIndex'):        
        input_targets = raw_index_task.input()[0]['collection'].targets.values()

        return [f.path for f in input_targets]
    
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

# Add them to the registry
zhh_configs.add(Config_500_all_full())
zhh_configs.add(Config_550_hh_fast())
zhh_configs.add(Config_550_hh_full())