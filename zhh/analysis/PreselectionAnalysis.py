from ast import literal_eval as make_tuple
from glob import glob
from dateutil import parser
from typing import Optional, Union, Iterable, List, Callable
from tqdm.auto import tqdm
import os.path as osp
import json
import numpy as np
import uproot as ur
import awkward as ak
from .TTreeInterface import TTreeInterface

DEFAULTS = {
    'PROD_NAME': '500-TDR_ws',
    'ILD_VERSION': 'ILD_l5_o1_v02'
}

def get_preselection_meta(DATA_ROOT:str)->dict:
    metafile = glob(f'{DATA_ROOT}/htcondor_jobs*.json')[0]
    with open(metafile) as file:
        meta = json.load(file)
        
    return meta

def file_get_polarization(src_path:str)->tuple[int, int]:
    pol_e = -1 if '.eL.p' in src_path else (+1 if '.eR.p' in src_path else 0)
    pol_p = -1 if '.pL.' in src_path else (+1 if '.pR.' in src_path else 0)
    
    if pol_e == 0 or pol_p == 0:
        print(f'Warning: Encountered 0 polarization (?)')
        
    return pol_e, pol_p

def parse_sample_path(src_path:str,
                    PROD_NAME:str=DEFAULTS['PROD_NAME'],
                    ILD_VERSION:str=DEFAULTS['ILD_VERSION'])->tuple:
    loc = src_path.split(f'/{PROD_NAME}/')[1]
    loc = loc.split(f'/{ILD_VERSION}/')[0]
    polarization = file_get_polarization(src_path)
    
    return (loc, polarization)

def parse_json(json_path:str):
    with open(json_path, 'r') as file:
        content = json.load(file)
        
    return content

def get_preselection_summary_for_branch(
            DATA_ROOT:str,
            branch:Union[int,str],
            PROD_NAME:str=DEFAULTS['PROD_NAME'],
            ILD_VERSION:str=DEFAULTS['ILD_VERSION']):
    
    branch = int(branch)
    
    try:
        with open(f'{DATA_ROOT}/{branch}/Source.txt') as file:
            src_path = file.read().strip()            
    except:
        src_path = ''
    
    try: 
        with open(f'{DATA_ROOT}/{branch}/zhh_FinalStateMeta.json') as jsonfile:
            fs_meta = json.load(jsonfile)
            process = fs_meta['processName']  
    except:
        process = ''
    
    try:
        with open(f'{DATA_ROOT}/stdall_{branch}To{branch+1}.txt') as file:
            # parse start time, end time, exit code to list of int/float (values)
            signals = ['start time    :', 'end time      :', 'job exit code :']
            temp = ["", "", ""]
            values:list[int|float] = [0, 0, 0]
            lsig = len(signals)
            
            for line in file.readlines():
                for i in range(lsig):
                    if line.startswith(signals[i]):
                        temp[i] = line.split(f'{signals[i]} ')[1].strip()
                    elif src_path == '' and '--global.LCIOInputFiles=' in line:
                        src_path = line.split('--global.LCIOInputFiles=')[1].strip().split(' --constant.OutputDirectory=')[0]
                        
            for i in [0, 1]:
                if temp[i] != '':
                    if ' (' in temp[i]:
                        temp[i] = temp[i].split(' (')[0]
                    
                    values[i] = float(parser.parse(temp[i]).timestamp())
            
            # exit code
            values[2] = int(temp[2])
                    
    except:
        values = [0, 0, 0]
    
    loc = ''
    polarization = (0, 0)
    if src_path != '':
        loc, polarization = parse_sample_path(src_path, PROD_NAME=PROD_NAME, ILD_VERSION=ILD_VERSION)
            
    return (branch, loc, process, polarization[0], polarization[1], src_path, values[0], values[1], values[1] - values[0], values[2])

def get_preselection_summary(DATA_ROOT:str, meta:dict)->np.ndarray:
    """_summary_

    Args:
        meta (dict): result returned from get_preselection_meta()

    Returns:
        np.ndarray: _description_
    """
    
    jobs = meta['jobs']
    dtype = [
        ('status', '<U16'),
        ('branch', 'I'),
        ('loc', '<U32'),
        ('process', '<U32'),
        ('pol_e', 'B'),
        ('pol_p', 'B'),
        ('src', '<U255'),
        ('tStart', 'f'),
        ('tEnd', 'f'),
        ('tDuration', 'f'),
        ('exitCode', 'i')]
    
    results = np.empty(0, dtype=dtype)

    for job_key in jobs:
        branch = jobs[job_key]['branches'][0]
        status = jobs[job_key]['status']
        
        ev = get_preselection_summary_for_branch(DATA_ROOT, branch)
        entry = (status, *ev)

        results = np.append(results, np.array([entry], dtype=dtype))
    
    return results

# https://doi.org/10.1016/j.physrep.2007.12.003
def w_prefacs(Pem, Pep):
    return (
        (1-Pem)*(1-Pep)/4, # LL
        (1-Pem)*(1+Pep)/4, # LR
        (1+Pem)*(1-Pep)/4, # RL
        (1+Pem)*(1+Pep)/4 # RR
    )
    

w_em_ep = {
    'LL': 0.315,
    'LR': 0.585,
    'RL': 0.035,
    'RR': 0.065
}

def combined_cross_section(processes:np.ndarray, process:Union[str, List[str]],
                           pol_em:float=-0.8, pol_ep:float=0.3)->float:
    
    if isinstance(process, list):
        return np.sum([combined_cross_section(processes, p, pol_em, pol_ep) for p in process])
    
    prefacs = w_prefacs(pol_em, pol_ep)
    cross_secs = np.zeros(4, dtype=float)
    
    for i, suffix in enumerate(['LL', 'LR', 'RL', 'RR']):
        entry = processes[processes['proc_pol'] == f'{process}_{suffix}']
        if len(entry) == 1:
            cross_secs[i] = entry['cross_sec'][0]
    
    return np.dot(prefacs, cross_secs)

def get_pol_key(pol_em:float, pol_ep:float)->str:
    key_em = ('L' if pol_em == -1. else ('R' if pol_em == 1. else 'N'))
    key_ep = ('L' if pol_ep == -1. else ('R' if pol_ep == 1. else 'N'))
    
    if key_em == 'N' or key_ep == 'N':
        raise Exception('Invalid polarization encountered')
    
    return key_em + key_ep

def get_w_pol(pol_em:int, pol_ep:int)->float:
    key = get_pol_key(pol_em, pol_ep)
    
    if not (key in w_em_ep):
        raise Exception(f'Unhandled polarization {key}')
    
    return w_em_ep[key]

def sample_weight(process_sigma_fb:float,
                  pol:tuple[int, int],
                  n_gen:int=1,
                  lum_inv_ab:float=2.)->float:
    
    w_pol = get_w_pol(*pol)    
    return process_sigma_fb * 1000 *lum_inv_ab * w_pol / n_gen

def get_preselection_passes(
    DATA_ROOT:str,
    version:str='v1')->np.ndarray:
    
    dtype = [
        ('process', '<U60'),
        ('pol_e', 'i'),
        ('pol_p', 'i'),
        ('n_gen', 'i'),
        ('cross_sec', 'f'),
        ('cross_sec_err', 'f'),
        ('n_pass_llhh', 'i'),
        ('n_pass_vvhh', 'i'),
        ('n_pass_qqhh', 'i'),
        ('weight', 'f'),
        ('proc_pol', '<U64')]

    results = np.empty(0, dtype=dtype)
    finished = glob(f'{DATA_ROOT}/*/Source.txt')
    finished.sort()

    for f in (pbar := tqdm(finished)):
        branch = f.split(f'{version}/')[1].split('/Source.txt')[0]
        
        if osp.isfile(f'{DATA_ROOT}/{branch}/zhh_FinalStateMeta.json'):
            # Source file is written at the very end -> .root files exist
            
            with open(f, 'r') as file:
                src_spec = file.read()
                if src_spec.startswith('('):
                    src_file, chunk_start, chunk_end = make_tuple(src_spec)
                else:
                    src_file = src_spec
                
            # Read metadata
            with open(f'{DATA_ROOT}/{branch}/zhh_FinalStateMeta.json', 'r') as file:
                meta = json.load(file)
                
            n_gen = meta['nEvtSum']
            proc = meta["processName"]
            
            loc, pol = parse_sample_path(src_file)
            procpol = f'{proc}_{get_pol_key(*pol)}'
            
            pbar.set_description(f'Adding {n_gen} events from branch {branch} ({procpol})')
            
            if not procpol in results['proc_pol']:
                results = np.append(results, [np.array([
                    (proc, pol[0], pol[1], n_gen, meta['crossSection'], meta['crossSectionError'], 0, 0, 0, 0., procpol)
                ], dtype=dtype)])
            else:
                results['n_gen'][results['proc_pol'] == procpol] += n_gen
                
            for presel in ['llHH', 'vvHH', 'qqHH']:
                with ur.open(f'{DATA_ROOT}/{branch}/zhh_PreSelection_{presel}.root') as rf:
                    passed = rf['PreSelection']['preselPassed'].array()
                    
                    n_items = len(passed)
                    if n_items != n_gen:
                        raise Exception('Constraint mismatch')
                    
                    n_passed = np.sum(passed)
                    results[f'n_pass_{presel.lower()}'][results['proc_pol'] == procpol] += n_passed
                    
    for entry in results:
        pol = (entry['pol_e'], entry['pol_p'])
        results['weight'][results['proc_pol'] == entry['proc_pol']] = sample_weight(entry['cross_sec'], pol, entry['n_gen'])

    results = results[np.argsort(results['proc_pol'])]
    
    return results

def extract_dijet_masses(dijetMass:ur.TBranch)->tuple[np.ndarray, np.ndarray]:
    aka = dijetMass.array()
    mask = ak.num(aka) > 0

    mh1 = np.array(ak.fill_none(aka.mask[mask][:, 0], -1), dtype='f')
    mh2 = np.array(ak.fill_none(aka.mask[mask][:, 1], -1), dtype='f')
    
    return mh1, mh2

def extract_b_tagging_values(bTags:ur.TBranch)->tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    btags = ak.sort(bTags.array(), ascending=False)

    return (
        np.array(ak.fill_none(btags.mask[ak.num(btags) > 0][:, 0], -1), dtype='f'),
        np.array(ak.fill_none(btags.mask[ak.num(btags) > 1][:, 1], -1), dtype='f'),
        np.array(ak.fill_none(btags.mask[ak.num(btags) > 2][:, 2], -1), dtype='f'),
        np.array(ak.fill_none(btags.mask[ak.num(btags) > 3][:, 3], -1), dtype='f')
    )

def analysis_stack(DATA_ROOT:str,
                 processes:np.ndarray,
                 chunks_f:np.ndarray,
                 branches:list,
                 kinematics:bool=False,
                 b_tagging:bool=False,
                 final_states:bool=False)->np.ndarray:   
            
    dtype = [
        ('branch', chunks_f.dtype['branch']), # np.uint32
        ('pid', 'H'), # np.uint16
        ('event', 'I'), # max. encountered: 15 797 803 << 4 294 967 295 (max of uint32)
        ('event_category', 'B'), # np.uint8
        
        ('ll_pass', 'B'),
        ('vv_pass', 'B'),
        ('qq_pass', 'B'),
    ]
    
    if kinematics:
        for dt in [
            ('thrust', 'f'),
            ('e_vis', 'f'),
            ('pt_miss', 'f'),
            ('invmass_miss', 'f'),
            ('nisoleps', 'B'),
        
            # llHH
            ('ll_mh1', 'f'),
            ('ll_mh2', 'f'),
            ('ll_nbjets', 'B'),
            
            ('ll_dilepton_type', 'B'),
            ('ll_mz', 'f'),
            ('ll_mz_pre_pairing', 'f'),
            
            # vvHH
            ('vv_mh1', 'f'),
            ('vv_mh2', 'f'),
            ('vv_nbjets', 'B'),
            
            ('vv_mhh', 'f'),
            
            # qqHH
            ('qq_mh1', 'f'),
            ('qq_mh2', 'f'),
            ('qq_nbjets', 'B')
        ]:
            dtype.append(dt)
            
    if b_tagging:
        for hyp in ['ll', 'vv', 'qq']:
            for dt in [
                (f'{hyp}_bmax1', 'f'),
                (f'{hyp}_bmax2', 'f'),
                (f'{hyp}_bmax3', 'f'),
                (f'{hyp}_bmax4', 'f')
            ]:
                dtype.append(dt)
                
    if final_states:
        for dt in fs_columns:
            dtype.append((dt, 'B'))
        
        dtype.append(('Nb_from_H', 'B'))
    
    r_size = chunks_f[np.isin(chunks_f['branch'], branches)]['chunk_size_factual'].sum()
    results = np.zeros(r_size, dtype=dtype)
    
    pointer = 0
    
    for branch in tqdm(branches):
        chunk_size = chunks_f[chunks_f['branch'] == branch]['chunk_size_factual'][0]
        chunk = np.zeros(chunk_size, dtype=dtype)
        
        proc_pol = chunks_f[chunks_f['branch'] == branch]['proc_pol'][0]
        chunk['pid'] = processes[processes['proc_pol'] == proc_pol]['pid'][0]
        chunk['branch'] = branch
        
        with ur.open(f'{DATA_ROOT}/{branch}/zhh_FinalStates.root:FinalStates') as rf:
            chunk['event'] = rf['event'].array()
            chunk['event_category'] = rf['event_category'].array()
            
            if final_states:
                fs_counts = rf['final_state_counts'][1].array()
                chunk['Nb_from_H'] = rf['n_b_from_higgs'].array()
                
                for i in range(len(fs_columns)):
                    chunk[fs_columns[i]] = fs_counts[:, i]
        
        for i, presel in enumerate(['qq', 'll', 'vv']):
            with ur.open(f'{DATA_ROOT}/{branch}/zhh_PreSelection_{presel}HH.root:PreSelection') as rf:
                chunk[f'{presel}_pass'] = rf['preselPassed'].array()
                
                if i == 0:
                    if kinematics:
                        chunk['thrust'] = rf['thrust'].array()
                        chunk['e_vis'] = rf['Evis'].array()
                        chunk['pt_miss'] = rf['missingPT'].array()
                        chunk['invmass_miss'] = rf['missingInvMass'].array()
                        chunk['nisoleps'] = rf['nIsoLeptons'].array()
                    
                if kinematics:
                    chunk[f'{presel}_nbjets'] = rf['nbjets'].array()
                    
                    mh1, mh2 = extract_dijet_masses(rf['dijetMass'])
                    chunk[f'{presel}_mh1'] = mh1
                    chunk[f'{presel}_mh2'] = mh2
                    
                    if presel == 'll':                        
                        #lepTypes = rf['lepTypes'].array()
                        #pass_ltype11 = np.sum(np.abs(lepTypes) == 11, axis=1) == 2
                        #pass_ltype13 = np.sum(np.abs(lepTypes) == 13, axis=1) == 2
                        #chunk['ll_dilepton_type'] = pass_ltype11*11 + pass_ltype13*13
                        lepTypesPaired = rf['lepTypesPaired'].array()
                        chunk['ll_dilepton_type'] = lepTypesPaired
                        
                        chunk['ll_mz'] = rf['dileptonMass'].array()
                        chunk['ll_mz_pre_pairing'] = rf['dileptonMassPrePairing'].array()
                        
                    elif presel == 'vv':
                        chunk['vv_mhh'] = rf['dihiggsMass'].array()
                
                if b_tagging:
                    chunk[f'{presel}_bmax1'], chunk[f'{presel}_bmax2'], chunk[f'{presel}_bmax3'], chunk[f'{presel}_bmax4'] = extract_b_tagging_values(rf['bTags'])
                    
        results[pointer:(pointer+chunk_size)] = chunk
        
        pointer += chunk_size

    return results

def fetch_preselection_data(rf, presel:str, final_states:bool=True, tree:str|None=None, fsTree:str='FinalStates', evtObsTree:str='EventObservables')->np.ndarray:
    if tree is not None:
        fsTree = tree
        evtObsTree = tree
    else:
        evtObsTree = f'EventObservables{presel.upper()}'
        
    #result = LazyTablelike(tree.num_entries)
    
    dtype = [
        ('id', 'I'),
        ('process', 'I'), # H=np.uint16
        ('pid', 'I'),
        ('pol_code', 'B'), # np.uint8
        ('event', 'I'), # max. encountered: 15 797 803 << 4 294 967 295 (max of uint32)
        ('event_category', 'B'), # np.uint8
        
        ('is_sig', '?'),
        ('is_bkg', '?'),
        
        ('ll_pass', 'B'),
        ('vv_pass', 'B'),
        ('qq_pass', 'B'),
        
        ('thrust', 'f'),
        ('e_vis', 'f'),
        ('pt_miss', 'f'),
        ('invmass_miss', 'f'),
        ('nisoleps', 'B'),
        ('xx_paired_isoleptype', 'B'),
        
        ('passed', 'B'),
        ('weight', 'f'),
        
        # ll
        ('ll_mh1', 'f'),
        ('ll_mh2', 'f'),
        ('ll_dilepton_type', 'B'),
        ('ll_mz', 'f'),
        ('ll_mz_pre_pairing', 'f'),
        
        # vv
        ('vv_mh1', 'f'),
        ('vv_mh2', 'f'),
        ('vv_mhh', 'f'),
        
        # qq
        ('qq_mh1', 'f'),
        ('qq_mh2', 'f'),
    ]
    
    dtype.extend([
        (f'{presel}_bmax1', 'f'),
        (f'{presel}_bmax2', 'f'),
        (f'{presel}_bmax3', 'f'),
        (f'{presel}_bmax4', 'f')
    ])
            
    if final_states:
        for dt in fs_columns:
            dtype.append((dt, 'B'))
        
        dtype.append(('Nb_from_H', 'B'))
    
    r_size = rf[fsTree].num_entries
    results = np.zeros(r_size, dtype=dtype)
    
    results['id'] = np.arange(r_size)
    results['process'] = rf[f'{fsTree}/process'].array()
    results['pol_code'] = rf[f'{fsTree}/polarization_code'].array()
    
    results['event'] = rf[f'{fsTree}/event'].array()
    results['event_category'] = rf[f'{fsTree}/event_category'].array()
        
    if final_states:
        fs_counts = rf[f'{fsTree}/final_state_counts'][1].array()
        results['Nb_from_H'] = rf[f'{fsTree}/n_b_from_higgs'].array()
        
        for i in range(len(fs_columns)):
            results[fs_columns[i]] = fs_counts[:, i]
    
    results['thrust'] = rf[f'{evtObsTree}/thrust'].array()
    results['e_vis'] = rf[f'{evtObsTree}/evis'].array()
    results['pt_miss'] = rf[f'{evtObsTree}/ptmiss'].array()
    results['invmass_miss'] = rf[f'{evtObsTree}/m_miss'].array()
    results['nisoleps'] = rf[f'{evtObsTree}/nisoleptons'].array()
    
    #KinFitTree = f'KinFit{presel.upper()}_ZHH'
    
    results[f'{presel}_mh1'] = rf[f'{evtObsTree}/zhh_mh1'].array()
    results[f'{presel}_mh2'] = rf[f'{evtObsTree}/zhh_mh2'].array()
    
    if presel == 'll':                        
        #lepTypes = rf['lepTypes'].array()
        #pass_ltype11 = np.sum(np.abs(lepTypes) == 11, axis=1) == 2
        #pass_ltype13 = np.sum(np.abs(lepTypes) == 13, axis=1) == 2
        #results['ll_dilepton_type'] = pass_ltype11*11 + pass_ltype13*13
        results['ll_dilepton_type'] = rf[f'{evtObsTree}/paired_lep_type'].array()
        results['ll_mz'] = rf[f'{evtObsTree}/mzll'].array()
        results['ll_mz_pre_pairing'] = rf[f'{evtObsTree}/mzll_pre_pairing'].array()
        
    elif presel == 'vv':
        results['vv_mhh'] = rf[f'{evtObsTree}/m_invjet'].array()
    
    results[f'{presel}_bmax1'] = rf[f'{evtObsTree}/bmax1']
    results[f'{presel}_bmax2'] = rf[f'{evtObsTree}/bmax2']
    results[f'{presel}_bmax3'] = rf[f'{evtObsTree}/bmax3']
    results[f'{presel}_bmax4'] = rf[f'{evtObsTree}/bmax4']
            
    return results


def fetch_preselection_data_old(rf, presel:str, final_states:bool=True, tree:str|None=None, fsTree:str='FinalStates', evtObsTree:str='EventObservables')->np.ndarray:
    if tree is not None:
        fsTree = tree
        evtObsTree = tree
    else:
        evtObsTree = f'EventObservables{presel.upper()}'
    
    dtype = [
        ('id', 'I'),
        ('process', 'I'), # H=np.uint16
        ('pid', 'I'),
        ('pol_code', 'B'), # np.uint8
        ('event', 'I'), # max. encountered: 15 797 803 << 4 294 967 295 (max of uint32)
        ('event_category', 'B'), # np.uint8
        
        ('is_sig', '?'),
        ('is_bkg', '?'),
        
        ('ll_pass', 'B'),
        ('vv_pass', 'B'),
        ('qq_pass', 'B'),
        
        ('thrust', 'f'),
        ('e_vis', 'f'),
        ('pt_miss', 'f'),
        ('invmass_miss', 'f'),
        ('nisoleps', 'B'),
        ('xx_paired_isoleptype', 'B'),
        
        ('passed', 'B'),
        ('weight', 'f'),
        
        # ll
        ('ll_mh1', 'f'),
        ('ll_mh2', 'f'),
        ('ll_dilepton_type', 'B'),
        ('ll_mz', 'f'),
        ('ll_mz_pre_pairing', 'f'),
        
        # vv
        ('vv_mh1', 'f'),
        ('vv_mh2', 'f'),
        ('vv_mhh', 'f'),
        
        # qq
        ('qq_mh1', 'f'),
        ('qq_mh2', 'f'),
    ]
    
    dtype.extend([
        (f'{presel}_bmax1', 'f'),
        (f'{presel}_bmax2', 'f'),
        (f'{presel}_bmax3', 'f'),
        (f'{presel}_bmax4', 'f')
    ])
            
    if final_states:
        for dt in fs_columns:
            dtype.append((dt, 'B'))
        
        dtype.append(('Nb_from_H', 'B'))
    
    r_size = rf[fsTree].num_entries
    results = np.zeros(r_size, dtype=dtype)
    
    results['id'] = np.arange(r_size)
    results['process'] = rf[f'{fsTree}/process'].array()
    results['pol_code'] = rf[f'{fsTree}/polarization_code'].array()
    
    results['event'] = rf[f'{fsTree}/event'].array()
    results['event_category'] = rf[f'{fsTree}/event_category'].array()
        
    if final_states:
        fs_counts = rf[f'{fsTree}/final_state_counts'][1].array()
        results['Nb_from_H'] = rf[f'{fsTree}/n_b_from_higgs'].array()
        
        for i in range(len(fs_columns)):
            results[fs_columns[i]] = fs_counts[:, i]
    
    results['thrust'] = rf[f'{evtObsTree}/thrust'].array()
    results['e_vis'] = rf[f'{evtObsTree}/evis'].array()
    results['pt_miss'] = rf[f'{evtObsTree}/ptmiss'].array()
    results['invmass_miss'] = rf[f'{evtObsTree}/m_miss'].array()
    results['nisoleps'] = rf[f'{evtObsTree}/nisoleptons'].array()
    
    #KinFitTree = f'KinFit{presel.upper()}_ZHH'
    
    results[f'{presel}_mh1'] = rf[f'{evtObsTree}/zhh_mh1'].array()
    results[f'{presel}_mh2'] = rf[f'{evtObsTree}/zhh_mh2'].array()
    
    if presel == 'll':                        
        #lepTypes = rf['lepTypes'].array()
        #pass_ltype11 = np.sum(np.abs(lepTypes) == 11, axis=1) == 2
        #pass_ltype13 = np.sum(np.abs(lepTypes) == 13, axis=1) == 2
        #results['ll_dilepton_type'] = pass_ltype11*11 + pass_ltype13*13
        results['ll_dilepton_type'] = rf[f'{evtObsTree}/paired_lep_type'].array()
        results['ll_mz'] = rf[f'{evtObsTree}/mzll'].array()
        results['ll_mz_pre_pairing'] = rf[f'{evtObsTree}/mzll_pre_pairing'].array()
        
    elif presel == 'vv':
        results['vv_mhh'] = rf[f'{evtObsTree}/m_invjet'].array()
    
    results[f'{presel}_bmax1'] = rf[f'{evtObsTree}/bmax1']
    results[f'{presel}_bmax2'] = rf[f'{evtObsTree}/bmax2']
    results[f'{presel}_bmax3'] = rf[f'{evtObsTree}/bmax3']
    results[f'{presel}_bmax4'] = rf[f'{evtObsTree}/bmax4']
            
    return results

fs_columns = ['Nd', 'Nu', 'Ns', 'Nc', 'Nb', 'Nt', 'Ne1', 'Nv1', 'Ne2', 'Nv2', 'Ne3', 'Nv3', 'Ng', 'Ny', 'NZ', 'NW', 'NH']

class PDG2FSCMap:
    d = 0
    u = 1
    s = 2
    c = 3
    b = 4
    t = 5
    e1 = 6
    v1 = 7
    e2 = 8
    v2 = 9
    e3 = 10
    v3 = 11
    g = 12
    y = 13
    Z = 14
    W = 15
    H = 16

PDG2FSC = PDG2FSCMap()

def apply_order(categories:List[str], order:Union[List[str], Callable]):
    if isinstance(order, Callable):
        return order(categories)
    elif isinstance(order, List):
        categories_new = order
        for cat in categories:
            if not cat in categories_new:
                categories_new.append(cat)
    
        return categories_new

def weighted_counts_by_categories(presel_results:TTreeInterface, categories_selected:Optional[np.ndarray]=None):
    
    categories = np.array(categories_selected) if categories_selected is not None else np.unique(presel_results['event_category'])
    counts = np.zeros(len(categories), dtype=float)
    
    for i, cat in enumerate(categories):
        counts[i] += presel_results['weight'][presel_results['event_category'] == cat].sum()
        
    return categories, counts

def calc_preselection_by_event_categories(presel_results:TTreeInterface, processes:np.ndarray,
                                          quantity:Optional[str]=None,
                                          order:Optional[Union[List[str],Callable]]=None,
                                          categories_selected:Optional[List[int]]=None,
                                          categories_additional:Optional[int]=3,
                                          weighted:bool=True,
                                          categories:Optional[np.ndarray]=None, counts:Optional[np.ndarray]=None,
                                        xlim:Optional[tuple]=None)->dict[str, np.ndarray]:
    
    """Processes event categories. For a given TTreeInterface, in the default case,
    for all event categories, a list of events processes with weight and selected quantity
    (which must exist in TTreeInterface) is returned. categories_selected and
    categories_additional can be used to select the event categories to be included dyna-
    mically by their importance. The order of results will be descending with respect to
    the sum of event weights.

    Args:
        presel_results (np.ndarray): _description_
        processes (np.ndarray): _description_  
        weighted (bool, optional): Whether the correct proc_pol weighting should be used. Defaults to True.
        quantity (str, optional): _description_. Defaults to 'll_mz'.
        categories_selected (list, optional): Event categories to include in any case. Defaults to [17].
        categories_additional (Optional[int], optional): n-th most contributing categories to include as well. Defaults to 3.
        order (Optional[List, Callable]): Either Callable that sorts the given categories or List which may be used for sorting. Defaults to None.
        unit (str, optional): _description_. Defaults to 'GeV'.
        nbins (int, optional): _description_. Defaults to 100.
        xlim (Optional[tuple], optional): _description_. Defaults to None.
        yscale (Optional[str], optional): _description_. Defaults to None.
        ild_style_kwargs (dict, optional): _description_. Defaults to {}.

    Returns:
        _type_: _description_
    """
    
    from ..processes.EventCategories import EventCategories
    if categories_additional == 0:
        categories_additional = None
    
    if xlim is not None and isinstance(quantity, str):
        subset = presel_results[(presel_results[quantity] > xlim[0]) & (presel_results[quantity] < xlim[1])]
    else:
        subset = presel_results
    
    # Find relevant event categories
    if weighted:
        if categories is None or counts is None:
            categories = np.array(categories_selected) if (categories_additional is None and categories_selected is not None) else np.unique(subset['event_category'])
            categories, counts = weighted_counts_by_categories(presel_results, categories)
        else:
            if categories_selected is not None and categories_additional is None:
                mask = np.isin(categories, categories_selected)
                categories, counts = categories[mask], counts[mask]
    else:
        categories, counts = np.unique(subset['event_category'], return_counts=True)
        
    # Sort and include categories_additional (descending)
    if categories_additional is not None:
        count_sort_ind = np.argsort(-counts)
        counts, categories = counts[count_sort_ind], categories[count_sort_ind]
        
        mask = np.isin(categories, categories_selected) if categories_selected is not None else np.zeros(len(categories), dtype='?')
        
        n_additional = 0
        for i in range(len(categories)):
            if not mask[i]:
                if n_additional < categories_additional:
                    mask[i] = True
                    n_additional += 1
                else:
                    break
                
        categories, counts = categories[mask], counts[mask]
    
    # Sort and include categories_additional (ascending for correct plotting)
    count_sort_ind = np.argsort(counts)
    counts, categories = counts[count_sort_ind], categories[count_sort_ind]
    
    if order is not None:
        if isinstance(order, list):
            order = order.copy()
            
        categories = apply_order(categories, order)
    
    calc_dict  = {}
    
    for category in (pbar := tqdm(categories)):
        label = EventCategories.inverted[category]
        pbar.set_description(f'Processing event category {label}')
        
        covered_processes = []

        mask = (subset['event_category'] == category)
        pids = np.unique(subset['pid'][mask])
        
        process_masks = []
        
        # Collect contributions from all proc_pol combinations of a process
        for pid in pids:
            process_name = processes['process'][processes['pid'] == pid][0]
            
            if process_name in covered_processes:
                continue
            else:
                covered_processes.append(process_name)
            
            mask_process = np.zeros(len(mask), dtype='?')
            
            for pidc in processes['pid'][processes['process'] == process_name]:
                #print(f"> {processes['proc_pol'][processes['pid'] == pidc][0]}")
                mask_process = mask_process | (subset['pid'] == pidc)
            
            mask_process = mask & mask_process
            
            process_masks.append(mask_process)
        
        category_mask = np.logical_or.reduce(process_masks)
        
        if quantity is None:
            calc_dict[label] = (None, subset['weight'][category_mask])
        else:
            calc_dict[label] = (subset[quantity][category_mask], subset['weight'][category_mask])
            
    return calc_dict


def calc_preselection_by_processes(presel_results:np.ndarray, processes:np.ndarray, weights:np.ndarray,
                                    weighted:bool=True,
                                    quantity:Optional[str]=None,
                                    processes_selected:Optional[List]=None,
                                    processes_additional:Optional[int]=None,
                                    order:Optional[List]=None,
                                    xlim:Optional[tuple]=None,
                                    check_hypothesis:Optional[str]=None)->Iterable[dict]:
    
    """Calculate the preselection results for a given hypothesis by event categories

    Args:
        presel_results (np.ndarray): _description_
        weights (np.ndarray): _description_
        processes (list, optional): _description_..
        quantity (str, optional): _description_. Defaults to 'll_mz'.
        mode (int): 0 for sorting by processes, 1 for sorting by event_categories
        weighted (bool, optional): Whether the correct proc_pol weighting should be used. Defaults to True.
        processes_selected (Optional[List[str]]): Processes to include in any case. Defaults to None.
        processes_additional (Optional[int], optional): n-th most-contributing processes are included. Defaults to None.
        order (Optional[List]): List of process names to sort by. Defaults to None.
        unit (str, optional): _description_. Defaults to 'GeV'.
        nbins (int, optional): _description_. Defaults to 100.
        xlim (Optional[tuple], optional): _description_. Defaults to None.
        check_hypothesis (str, optional): If either llHH, vvHH or qqHH, will check if events pass the respective total selection as well. Defaults to None.
        yscale (Optional[str], optional): _description_. Defaults to None.
        ild_style_kwargs (dict, optional): _description_. Defaults to {}.

    Returns:
        _type_: _description_
    """
    
    # Make sure that the weights are ordered exactly by index
    assert(np.sum(weights['pid'] == np.arange(len(weights))) == len(weights))
    
    if xlim is not None and isinstance(quantity, str):
        subset = presel_results[(presel_results[quantity] > xlim[0]) & (presel_results[quantity] < xlim[1])]
    else:
        subset = presel_results
    
    # Find relevant processes
    process_names = np.unique(processes['process'])
    process_counts = np.zeros(len(process_names), dtype=float if weighted else int)
        
    for i, process_name in enumerate(process_names):
        pids = processes['pid'][processes['process'] == process_name]
        
        if weighted:
            process_counts[i] += np.sum(np.isin(subset['pid'], pids) * weights['weight'][subset['pid']])
        else:
            process_counts[i] = np.sum(np.isin(subset['pid'], pids))
 
    count_sort_ind = np.argsort(-process_counts)
    process_names = process_names[count_sort_ind]
    
    mask = np.isin(process_names, processes_selected) if processes_selected is not None else np.zeros(len(process_names), dtype='?')
    
    if processes_additional is not None:
        n_additional = 0
        for i in range(len(process_names)):
            if not mask[i]:
                if n_additional < processes_additional:
                    mask[i] = True
                    n_additional += 1
                else:
                    break
                
    process_names = process_names[mask]
    
    calc_dict_all  = {}
    calc_dict_pass = {}
    
    for proccess_name in (pbar := tqdm(process_names)):
        pbar.set_description(f'Processing process {proccess_name}')
        
        process_masks = []
        
        # Collect contributions from all proc_pol combinations of a process
        for pid in processes['pid'][processes['process'] == proccess_name]:
            process_masks.append((subset['pid'] == pid))
        
        mask = np.logical_or.reduce(process_masks)
        
        if quantity is None:
            calc_dict_all[proccess_name] = (None, weights['weight'][subset['pid'][mask]])
        else:
            calc_dict_all[proccess_name] = (subset[quantity][mask], weights['weight'][subset['pid'][mask]])
        
        if check_hypothesis is not None:
            category_mask_pass = mask & subset[f'{check_hypothesis[:2]}_pass']
            if quantity is None:
                calc_dict_pass[proccess_name] = (None, weights['weight'][subset['pid'][category_mask_pass]])        
            else:
                calc_dict_pass[proccess_name] = (subset[quantity][category_mask_pass], weights['weight'][subset['pid'][category_mask_pass]])
    
    if check_hypothesis is not None:
        return [calc_dict_all, calc_dict_pass]
    else:
        return [calc_dict_all]
    
def subset_test(presel_results:np.ndarray, n_per_process:int=1000, mask_only:bool=False)->np.ndarray:
    """Returns a subset or mask of presel_results that includes min(n_per_process, available events)
    events per process. Useful for prototyping.

    Args:
        presel_results (np.ndarray): _description_
        n_per_process (int, optional): _description_. Defaults to 1000.
        mask_only (bool, optional): _description_. Defaults to False.

    Returns:
        np.ndarray: either the subset, or a mask
    """
    pids, counts = np.unique(presel_results['pid'], return_counts=True)

    mask_size = np.min(np.stack([n_per_process*np.ones(len(pids), dtype='I'), counts]), axis=0).sum()
    mask = np.zeros(mask_size, dtype='I')
    pointer = 0
    
    for i, pid in enumerate(pids):
        current_size = min(counts[i], n_per_process)
        mask[pointer:(pointer+current_size)] = np.where(presel_results['pid'] == pid)[0][:current_size]
        pointer += current_size
        
    if mask_only:
        return mask
    
    return presel_results[mask]