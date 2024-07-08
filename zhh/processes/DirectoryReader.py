from .Process import Process, MarlinResult
from glob import glob
import json
import os.path as osp           

def read_meta_file(meta_file:str)->tuple[Process,MarlinResult]:
    try:
        with open(meta_file, 'r') as f:
            meta = json.load(f)
        
        kwargs_process = {
            'process_id': meta['processId'],
            'process_name': meta['processName'],
            'pol_electron': meta['polElectron'],
            'pol_positron': meta['polPositron'],
            'cross_section': meta['crossSection'],
            'cross_section_err': meta['crossSectionError']
        }
        process = Process(**kwargs_process)
        
        prename = meta_file.split('_FinalStateMeta.json')
        if len(prename) != 2:
            raise ValueError(f'Meta file has unexpected name <{meta_file}>')
        
        prename = prename[0]
        kwargs_result = {
            'process': process,
            'run_id': meta['runId'],
            'n_events': meta['nEvtSum'],
            
            'final_state_meta_path': meta_file,
            'final_state_path': f'{prename}_FinalStates.root',
            
            'zhh_presel_llhh_path': f'{prename}_PreSelection_llHH.root',
            'zhh_presel_vvhh_path': f'{prename}_PreSelection_vvHH.root',
            'zhh_presel_qqhh_path': f'{prename}_PreSelection_qqHH.root',
        }
        result = MarlinResult(**kwargs_result)
        
        return (process, result)
    except:
        raise ValueError(f'Error reading {meta_file}')
    



