from analysis.framework import AnalysisConfiguration, zhh_configs
from typing import TYPE_CHECKING
from os import environ, path as osp
import numpy as np
from zhh import glob_exp

if TYPE_CHECKING:
    from analysis.tasks import RawIndex, AbstractIndex

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