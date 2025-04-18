from typing import Optional, Literal
from tqdm.auto import tqdm
from glob import glob
import uproot as ur
import numpy as np
import os.path as osp

from sklearn import metrics
from zhh import OptionDict
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

# tagsXXXX shape:
# 0: b tag
# 1: c tag
# 2: uds tag

def parse_files(fileAIDA:str, fileFinalStates:str|None=None,
                valid_jet_pdgs:list[int]=[1,2,3,4,5],
                exclude_pdgs:Optional[list[int]]=[21,23,24],
                elementon_pdg:Literal['initial', 'final']='initial'):
    """_summary_

    Args:
        fileFinalStates (str): _description_
        fileAIDA (str): _description_
        valid_jet_pdgs (list[int], optional): _description_. Defaults to [1,2,3,4,5].
        exclude_pdgs (list[int], optional): if not None, a list of PDGs that are checked to not exist. an Exception is raised if any are found. defaults to [21,23,24] -> no gluons,Z,W
        elementon_pdg (Literal['initial', 'final']): which elementon PDG to use. check the TrueJet documentation.
            initial_elementon is the MCParticle that is at the beginning of the parton shower leading to the matched TrueJet.
            final_elementon is the MCParticle that is at the end of the parton shower leading to the matched TrueJet.

    Returns:
        tuple: tot_length, y_true[valid_jets_mask], tags1, tags2, valid_jets_mask
            valid_jets_mask is used as a mask for using only the jets comming from valid_jet_pdgs jets
    """
    if exclude_pdgs is not None:
        with ur.open(fileFinalStates) as ff:
            fs_mask = np.array(ff['FinalStates']['passed'].array(), dtype=bool)
            fs_counts = np.array(ff['FinalStates']['final_state_counts.second'].array())
            
            pdgs = np.array(ff['FinalStates']['final_state_counts.first'].array(entry_stop=1))[0]
            for pdg in exclude_pdgs:
                pdg_index = np.where(pdgs == pdg)[0]
                assert(len(pdg_index) == 1)
                pdg_index = pdg_index[0]
                
                n_found = fs_counts[fs_mask, pdg_index].sum()
                print(f'{pdg}: {n_found}')
                if n_found > 0:
                    raise Exception(f'Found {n_found} entries of PDG {pdg} which was requested to be excluded')
        
    with ur.open(fileAIDA) as af:
        y_true = np.array([])
        valid_jets_mask = np.array([])
        tags = []
        if 'JetTaggingComparison;1' in af.keys() and 'Jets;1' in af.keys():
            y_true = np.abs(np.array(af['Jets'][f'TrueJet{"Initial" if elementon_pdg.lower() == "initial" else "Final"}ElementonPDG'].array()))
            valid_jets_mask = np.isin(y_true, valid_jet_pdgs)
            
            if not (np.sum(af['JetTaggingComparison']['energy'].array() == af['Jets']['JetEnergy'].array()) == len(valid_jets_mask)):
                raise Exception('Order of jets must match exactly')
        
            for branch in ['tags1', 'tags2', 'tags3']:
                if branch in af['JetTaggingComparison'].keys():
                    current_tags = np.array(af['JetTaggingComparison'][branch].array())
                    assert(len(y_true) == len(current_tags))
                    
                    tags.append(current_tags)
        else:
            print(f'Warning: File {fileAIDA} does either not contain the JetTaggingComparison or Jet TTree and will be skipped')
    
    return len(valid_jets_mask), y_true, tags, valid_jets_mask

def get_tot_length(paths:list[str], print_errors=True)->int:
    result = 0
    for path in paths:
        with ur.open(path) as rf:
            if 'JetTaggingComparison;1' in rf.keys() and 'Jets;1' in rf.keys():
                result += len(rf['JetTaggingComparison']['event'].array())
            elif print_errors:
                print(f'Warning: File {path} does either not contain the JetTaggingComparison or Jet TTree and will be skipped')
    
    return result

def load_ftag_results(filesAIDA:list[str], parse_kwargs={}):
    filesFinalStates:list[str] = []
    for file in filesAIDA:
        dn = osp.dirname(file)
        bn = osp.basename(file)
        
        totalsuffix = bn.replace('AIDAFile_', '').replace('.root', '')
        p = osp.join(dn, f'FinalStates_{totalsuffix}.root')
        
        filesFinalStates.append(p)
        
    tot_length = get_tot_length(filesAIDA, print_errors=False)
    pdgs = np.zeros(tot_length, dtype='B')
    valid_jets_mask = np.zeros(tot_length, dtype=bool)
    pointer_valid = 0
    pointer_all = 0
    
    tags = []
    for i, (fileFinalStates, fileAIDA) in enumerate(pbar := tqdm(list(zip(filesFinalStates, filesAIDA)))):
        pbar.set_description(fileAIDA)
        
        chunk_length, chunk_pdgs, chunk_tags, chunk_valid_jets_mask = parse_files(fileAIDA, fileFinalStates, **parse_kwargs)
        if chunk_length:
            for j in range(len(chunk_tags)):
                tags_n = chunk_tags[j]
                if i == 0:
                    tags.append(np.zeros((tot_length, tags_n.shape[1])))
                
                tags[j][pointer_valid:pointer_valid+chunk_length] = tags_n
            
            pdgs[pointer_valid:pointer_valid+chunk_length] = chunk_pdgs
            valid_jets_mask[pointer_all:pointer_all+chunk_length] = chunk_valid_jets_mask
            
        pointer_valid += chunk_length
        pointer_all += len(chunk_valid_jets_mask)
    
    y_true = np.zeros(len(pdgs), dtype='B')
    for needle, replace in [
        (1, 2),
        (2, 2),
        (3, 2),
        (4, 1),
        (5, 0)
    ]:
        y_true[pdgs == needle] = replace
    
    return y_true, tags, valid_jets_mask

roc_plot_config = {
    'options': OptionDict([
        ('xmin', 0),
        ('xmax', 1),
        ('ymin', 0),
        ('ymax', 1),
        ('yscale', 'linear'),
        ('title_pre', '')
    ])
}

def plot_roc_two_cases(y_true, tags, methods, to_plot,
                       plot_options={},
                       plot_defaults:OptionDict|None=None,
                       tag_names=['b', 'c', 'uds']):
    
    assert(len(tags) == len(methods))
    
    if plot_defaults is None:
        plot_defaults = roc_plot_config['options']
    
    figs = []
    rocs = {}
    
    for i, (sig_name, bkg_name) in enumerate(to_plot):
        sig_index, bkg_index = tag_names.index(sig_name), tag_names.index(bkg_name)
        mask = (y_true == sig_index) | (y_true == bkg_index)
        print(f'{sig_name} (ID{sig_index}, n={(y_true == sig_index).sum()}), {bkg_name} (ID{bkg_index}, n={(y_true == bkg_index).sum()})')

        fig, ax = plt.subplots(figsize=(6, 6))

        for preds, label in zip(tags, methods):
            y_true_subs = np.copy(y_true[mask])
            pred_sig_subs = np.copy(preds[mask, sig_index])
            
            mask_sig = y_true_subs == sig_index
            mask_bkg = y_true_subs == bkg_index
            y_true_subs[mask_sig] = 1
            y_true_subs[mask_bkg] = 0
            
            # invalid predictions: assume signal, but predicted as background
            mask_invalid = preds[mask].sum(axis=1) == 0
            #print(f'{(mask_invalid).sum()} invalid ({label})')
            
            y_true_subs[mask_invalid] = 1
            pred_sig_subs[mask_invalid] = 0
            
            # calculate roc curve and plot
            fpr, tpr, threshold = metrics.roc_curve(y_true_subs, pred_sig_subs)
            roc_auc = metrics.auc(fpr, tpr)

            rocs[f'{label}_{sig_name}_vs_{bkg_name}'] = roc_auc
            
            ax.plot(tpr, fpr, label=f'{label} {sig_name} vs {bkg_name} (AUC = {roc_auc:.4f})');
            ax.xaxis.set_minor_locator(ticker.FixedLocator(.1 + np.arange(5)*.2))

        ax.set_xlabel(f'True Positive ({sig_name}) Rate')
        ax.set_ylabel(f'False Positive ({bkg_name}) Rate')
        
        plot_opt = {} if i not in plot_options else plot_options[i]
        plot_opt = plot_defaults.merge(plot_opt)
        
        ax.set_xlim(plot_opt['xmin'], plot_opt['xmax'])
        ax.set_ylim(plot_opt['ymin'], plot_opt['ymax'])
        ax.set_yscale(plot_opt['yscale'])
        
        ax.set_title(f'{plot_opt["title_pre"]}{sig_name} vs {bkg_name}')
        ax.grid()
        ax.legend()
        
        figs.append(fig)

    return figs, rocs