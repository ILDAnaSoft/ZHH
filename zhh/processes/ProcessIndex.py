import os.path as osp
import numpy as np

from tqdm.auto import tqdm
from zhh.analysis import get_pol_key
from multiprocessing import Pool

class SampleMeta():   
    def __init__(self, process:str, n_events:int, run_number:int, \
                beamPol1:float, beamPol2:float, \
                beamPol1Alt:float, beamPol2Alt:float, \
                crossSection:float, crossSection_err:float, \
                process_id:int, mcpColName:str):
        
        self.process = process
        self.nEvtSum = n_events
        self.run = run_number
        self.beamPol1 = beamPol1
        self.beamPol2 = beamPol2
        self.beamPol1Alt = beamPol1Alt
        self.beamPol2Alt = beamPol2Alt
        self.crossSection = crossSection
        self.crossSection_err = crossSection_err
        self.process_id = process_id
        self.mcp_col_name = mcpColName
        
    @classmethod
    def fromevent(cls, event, n_events:int, run_number:int):
        "Initialize SampleMeta from LCIO event data"
        
        params = event.getParameters()
        
        col_names = event.getCollectionNames()
        mcp_col_name = ''
        for name in ['MCParticlesSkimmed', 'MCParticle']:
            if name in col_names:
                mcp_col_name = name
                
        if mcp_col_name == '':
            print('Warning: No collection storing MCParticles found! Empty string will be stored.')
        
        return cls(params.getStringVal('processName'), n_events, run_number, \
                   params.getFloatVal('Pol0'), params.getFloatVal('Pol1'), \
                   params.getFloatVal('beamPol1'), params.getFloatVal('beamPol2'), \
                   params.getFloatVal('crossSection'), params.getFloatVal('crossSectionError'), \
                   params.getIntVal('ProcessID'), mcp_col_name)

def per_chunk(input_entry:tuple[int, list[str]]) -> tuple[int, list[tuple[str, SampleMeta]]]:
    chunk_idx, file_paths = input_entry
    
    # Suppress LCIO output
    from os import devnull, path as osp
    import sys
    stdout = sys.stdout
    stderr = sys.stderr
    sys.stdout = sys.stderr = open(devnull, 'w')
    
    from pyLCIO import IOIMPL
    sys.stdout, sys.stderr = stdout, stderr
    
    #for location in (pbar := tqdm(file_paths)):
    #    pbar.set_description(f'Processing file {location}')
    
    result = []
    for location in (file_paths):
        if not osp.isfile(location):
            raise Exception(f'File {location} does not exist!')
        
        reader = IOIMPL.LCFactory.getInstance().createLCReader()
        reader.open(location)
        
        # treat case where number of events = 0; calls to readNextEvent() fails in this case
        # relevant when WhizardEventGeneration produces samples with no contribution
        if reader.getNumberOfEvents() > 0:
            event = reader.readNextEvent()
            
            file_meta = SampleMeta.fromevent(event, reader.getNumberOfEvents(), event.getRunNumber())
            
            result.append((location, file_meta))
        else:
            raise Exception(f'File {location} has no events! Cannot index sample.')
            result.append((location, None))
    
    return (chunk_idx, result)

class ProcessIndex:
    process_index: str = 'processes.json'
    samples_index: str = 'samples.json'
    
    dtype_sample = [
        ('sid', 'i'),
        ('run_id', 'i'),
        ('process', '<U60'),
        ('proc_pol', '<U64'),
        ('n_events', 'i'),
        ('pol_e', 'i'),
        ('pol_p', 'i'),
        ('location', '<U512'),
        ('mcp_col_name', '<U24')]

    dtype_process = [
        ('pid', 'i'),
        ('process', '<U60'),
        ('proc_pol', '<U64'),
        ('pol_e', 'i'),
        ('pol_p', 'i'),
        
        ('cross_sec', 'f'),
        ('cross_sec_err', 'f'),
        ('generator_id', 'i')]
    
    def __init__(self,
                 PROCESS_INDEX:str,
                 SAMPLE_INDEX:str,
                 RAW_FILE_LIST:list[str]):
        
        self.PROCESS_INDEX = PROCESS_INDEX
        self.SAMPLE_INDEX = SAMPLE_INDEX
        
        self.PROCESS_INDEX_TMP = osp.splitext(PROCESS_INDEX)[0] + '_tmp.npy'
        self.SAMPLE_INDEX_TMP = osp.splitext(SAMPLE_INDEX)[0] + '_tmp.npy'
        
        self.RAW_FILE_LIST = RAW_FILE_LIST
        self.STATE = 0
        
        if osp.isfile(PROCESS_INDEX) and osp.isfile(SAMPLE_INDEX):
            self.processes = np.load(PROCESS_INDEX)
            self.samples = np.load(SAMPLE_INDEX)
            self.STATE = 1
        else:
            self.samples = np.empty(0, dtype=self.dtype_sample)
            self.processes = np.empty(0, dtype=self.dtype_process)
        
    def load(self,
             CHUNK_SIZE:int=128,
             pbar:bool=True):
        
        if self.STATE == 1:
            return
        
        remaining_files:list[str] = list(set(self.RAW_FILE_LIST) - set(self.samples['location']))
        
        chunks = []
        if CHUNK_SIZE == 0:
            chunks.append((0, remaining_files))
        else:
            chunk_idx = 0
            for i in range(0, len(remaining_files), CHUNK_SIZE):
                chunks.append((chunk_idx, remaining_files[i:i+CHUNK_SIZE]))
                chunk_idx += 1
                
        print('Got', len(remaining_files), 'files to process in', len(chunks), 'chunks')
        
        n_process = 0
        n_sample = 0
        
        chunk_outputs = []
        with Pool() as pool:
            progress = tqdm(range(len(remaining_files)), disable=not pbar)
            
            for chunk_output in pool.imap_unordered(per_chunk, chunks):
                chunk_idx, chunk = chunk_output
                
                progress.set_description(f'Receiving data for chunk {chunk_idx} with {len(chunk)} files')
                progress.update(len(chunk))
                
                chunk_outputs += chunk
                        
                #self.save()
                
        chunk_outputs.sort(key=lambda x: x[0]) # Sort by file location
        
        meta:SampleMeta
        for location, meta in chunk_outputs:            
            if meta.beamPol1 != 0 or meta.beamPol2 != 0:
                pol_em, pol_ep = meta.beamPol1, meta.beamPol2
            else:
                pol_em, pol_ep = meta.beamPol1Alt, meta.beamPol2Alt
                
            process = meta.process
            
            proc_pol = f'{process}_{get_pol_key(pol_em, pol_ep)}'
            
            if not proc_pol in self.processes['proc_pol']:
                cx, cx_err = meta.crossSection, meta.crossSection_err
                self.processes = np.append(self.processes, [np.array([
                    (n_process, process, proc_pol, pol_em, pol_ep, cx, cx_err, meta.process_id)
                ], dtype=self.dtype_process)])
                n_process += 1
                
            if not location in self.samples['location']:
                self.samples = np.append(self.samples, [np.array([
                    (n_sample, meta.run, process, proc_pol, meta.nEvtSum, pol_em, pol_ep, location, meta.mcp_col_name)
                ], dtype=self.dtype_sample)])
                n_sample += 1
        
        self.STATE = 1
        self.save()
        print('Finished indexing processes and samples')

    def save(self):
        np.save(self.SAMPLE_INDEX if self.STATE == 1 else self.SAMPLE_INDEX_TMP, self.samples)
        np.save(self.PROCESS_INDEX if self.STATE == 1 else self.PROCESS_INDEX_TMP, self.processes)