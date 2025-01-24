from typing import Optional
from tqdm.auto import tqdm
from glob import glob
import uproot as ur
import numpy as np
import os.path as osp

# tagsXXXX shape:
# 0: b tag
# 1: c tag
# 2: uds tag

def parse_files(fileAIDA:str, fileFinalStates:str|None=None, valid_jet_pdgs:list[int]=[1,2,3,4,5], exclude_pgs:Optional[list[int]]=[21,23,24]):
    """_summary_

    Args:
        fileFinalStates (str): _description_
        fileAIDA (str): _description_
        valid_jet_pdgs (list[int], optional): _description_. Defaults to [1,2,3,4,5].
        exclude_pgs (list[int], optional):
            if not None, a list of PDGs that are checked to not exist. an Exception is raised if any are found. 
            defaults to [21,23,24] -> no gluons,Z,W


    Returns:
        tuple: tot_length, y_true[valid_jets_mask], tagsPNet[valid_jets_mask], tagsLCFI[valid_jets_mask], valid_jets_mask
            valid_jets_mask is used as a mask for using only the jets comming from valid_jet_pdgs jets
    """
    if exclude_pgs is not None:
        with ur.open(fileFinalStates) as ff:
            fs_mask = np.array(ff['FinalStates']['passed'].array(), dtype=bool)
            fs_counts = np.array(ff['FinalStates']['final_state_counts.second'].array())
            
            pdgs = np.array(ff['FinalStates']['final_state_counts.first'].array(entry_stop=1))[0]
            for pdg in exclude_pgs:
                pdg_index = np.where(pdgs == pdg)[0]
                assert(len(pdg_index) == 1)
                pdg_index = pdg_index[0]
                
                n_found = fs_counts[fs_mask, pdg_index].sum()
                if n_found > 0:
                    raise Exception(f'Found {n_found} entries of PDG {pdg} which was requested to be excluded')
        
    with ur.open(fileAIDA) as af:
        y_true = np.abs(np.array(af['Jets']['TrueJetInitialElementonPDG'].array()))
        valid_jets_mask = np.isin(y_true, valid_jet_pdgs)
        
        tagsML = np.array(af['JetTaggingComparison']['tags1'].array())
        tagsLCFI = np.array(af['JetTaggingComparison']['tags2'].array())
        
        assert(len(y_true) == len(tagsML))
    
    return len(valid_jets_mask), y_true, tagsML, tagsLCFI, valid_jets_mask

def get_tot_length(paths:list[str])->int:
    result = 0
    for path in paths:
        with ur.open(path) as rf:
            result += len(rf['JetTaggingComparison']['event'].array())
    
    return result

def load_ftag_results(filesAIDA:list[str], n_tagsML:int=4, n_tagsLCFI:int=3):
        
    filesFinalStates:list[str] = []
    for file in filesAIDA:
        dn = osp.dirname(file)
        bn = osp.basename(file)
        
        totalsuffix = bn.replace('AIDAFile_', '').replace('.root', '')
        p = osp.join(dn, f'FinalStates_{totalsuffix}.root')
        
        filesFinalStates.append(p)
        
    tot_length = get_tot_length(filesAIDA)
        
    tagsPNet = np.zeros((tot_length, n_tagsML))
    tagsLCFI = np.zeros((tot_length, n_tagsLCFI))
    pdgs = np.zeros(tot_length, dtype='B')
    valid_jets_mask = np.zeros(tot_length, dtype=bool)
    pointer_valid = 0
    pointer_all = 0

    for fileFinalStates, fileAIDA in (pbar := tqdm(list(zip(filesFinalStates, filesAIDA)))):
        pbar.set_description(fileAIDA)
        
        chunk_length, chunk_pdgs, chunk_tags_pnet, chunk_tags_lcfi, chunk_valid_jets_mask = parse_files(fileAIDA, fileFinalStates)
        
        pdgs[pointer_valid:pointer_valid+chunk_length] = chunk_pdgs
        tagsPNet[pointer_valid:pointer_valid+chunk_length] = chunk_tags_pnet
        tagsLCFI[pointer_valid:pointer_valid+chunk_length] = chunk_tags_lcfi
        valid_jets_mask[pointer_all:pointer_all+len(chunk_valid_jets_mask)] = chunk_valid_jets_mask
        
        pointer_valid += chunk_length
        pointer_all += len(chunk_valid_jets_mask)

    #tagsPNet = tagsPNet[:pointer_valid]
    #tagsLCFI = tagsLCFI[:pointer_valid]
    #valid_jets_mask = valid_jets_mask[:pointer_all]
    
    y_true = np.zeros(len(pdgs), dtype='B')
    for needle, replace in [
        (1, 2),
        (2, 2),
        (3, 2),
        (4, 1),
        (5, 0)
    ]:
        y_true[pdgs == needle] = replace
    
    return y_true, tagsPNet, tagsLCFI, valid_jets_mask