from glob import glob
from os import environ, path as osp 
from pyLCIO import IOIMPL
from zhh import is_readable
import json

SAMPLE_ROOT = '/pnfs/desy.de/ilc/prod/ilc/ild/copy/dst-merged/500-TDR_ws'
REPO_ROOT = '/afs/desy.de/user/b/bliewert/public/MarlinWorkdirs/ZHH'

all_files:list[str] = []
checked_files:dict[str,dict] = {}
checked_processes = {}

def get_all_files():
    return glob(f'{SAMPLE_ROOT}/**/*.slcio', recursive=True)

def save_index():
    global checked_files, checked_processes
    with open(f'{REPO_ROOT}/notebooks/checked_files.txt', 'w') as f:
        json.dump(checked_files, f, ensure_ascii=False, indent='\t')
        
    with open(f'{REPO_ROOT}/notebooks/checked_processes.txt', 'w') as f:
        json.dump(checked_processes, f, ensure_ascii=False, indent='\t')

def load_index():
    global checked_files, checked_processes
    with open(f'{REPO_ROOT}/notebooks/checked_files.txt', 'r') as f:
        checked_files = json.load(f)
        
    with open(f'{checked_processes}/notebooks/checked_processes.txt', 'r') as f:
        checked_processes = json.load(f)

if osp.isfile(f'{REPO_ROOT}/notebooks/checked_files.txt'):
    load_index()

all_files = list(set(get_all_files()) - set(checked_files.keys()))

# Loop
if __name__ == "__main__":
    for path in all_files:
        print(f'Info: Reading {path}')
        
        sample = path.split('/')[10]
        
        if is_readable(path):
            reader = IOIMPL.LCFactory.getInstance().createLCReader()
            reader.open(path)
            
            event = reader.readNextEvent()
            params = event.getParameters()
            process = params.getStringVal('processName')
            
            if not (sample in checked_processes):
                checked_processes[sample] = []                
                print(f'Info: Added sample {sample}')
            
            checked_files[path] = {
                'process': process,
                'nEvtSum': reader.getNumberOfEvents(),
                'run': event.getRunNumber(),
                'beamPol1': params.getFloatVal('Pol0'),
		        'beamPol2': params.getFloatVal('Pol1'),
		        'crossSection': params.getFloatVal('crossSection'),
		        'crossSection_err': params.getFloatVal('crossSectionError'),
		        'process_id': params.getIntVal('ProcessID'),
            }
            
            if not (process in checked_processes[sample]):
                checked_processes[sample].append(process)
                
                print(f'Info: Added process {process} for sample {sample}')
                save_index()
            
            reader.close()