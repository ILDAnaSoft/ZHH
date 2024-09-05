from typing import Optional, List, Tuple, Iterable
from tqdm.auto import tqdm
from .ild_style import fig_ild_style
from phc import module_reload
module_reload('phc')
from phc import plot_hist
import numpy as np
from matplotlib.figure import Figure

# For a given final state, plots the most important processes
def plot_preselection_by_event_category(presel_results:np.ndarray, processes:np.ndarray, weights:np.ndarray,
                                        hypothesis:str, event_categories:list=[17], quantity:str='ll_mz',
                                        unit:str='GeV', xlim:Optional[Iterable]=None, nbins:int=100, xlabel:Optional[str]=None)->List[Figure]:
    """Plot the preselection results for a given hypothesis by event categories

    Args:
        presel_results (np.ndarray): _description_
        weights (np.ndarray): _description_
        hypothesis (str): _description_
        event_categories (list, optional): _description_. Defaults to [17].

    Returns:
        _type_: _description_
    """
    
    if xlabel is None:
        xlabel = quantity
    
    all_figs = []
    hypothesis_key = hypothesis[:2]
    
    for category in event_categories:
        calc_dict_all  = {}
        calc_dict_pass = {}
        
        covered_processes = []

        mask = (presel_results['event_category'] == category) & (presel_results[quantity] > 0)
        pids = np.unique(presel_results['pid'][mask])
        
        # Collect contributions from all proc_pol combinations of a process
        for pid in (pbar := tqdm(pids)):
            process_name = processes['process'][processes['pid'] == pid][0]
            
            if process_name in covered_processes:
                continue
            else:
                covered_processes.append(process_name)
            
            #print(process_name)
            mask_process = np.zeros(len(mask), dtype='?')
            
            for pidc in processes['pid'][processes['process'] == process_name]:
                #print(f"> {processes['proc_pol'][processes['pid'] == pidc][0]}")
                mask_process = mask_process | (presel_results['pid'] == pidc)

            mask_process_pass = np.logical_and(mask_process, presel_results[f'{hypothesis_key}_pass'])
            
            # Assumes indices of weights = weights['pid']
            calc_dict_all [process_name] = (presel_results[quantity][mask_process], weights['weight'][presel_results['pid'][mask_process]])
            calc_dict_pass[process_name] = (presel_results[quantity][mask_process_pass], weights['weight'][presel_results['pid'][mask_process_pass]])
        
        # Sort by number of entries
        figs_all  = plot_preselection_by_process(calc_dict_all , unit, hypothesis, xlabel, nbins=nbins, xlim=xlim, title_label=rf'all events')
        figs_pass = plot_preselection_by_process(calc_dict_pass, unit, hypothesis, xlabel, nbins=nbins, xlim=xlim, title_label=rf'events passing {hypothesis}')
        
        all_figs += figs_all + figs_pass
        
    return all_figs

def plot_preselection_by_process(calc_dict, xunit:str, hypothesis:str, xlabel:str,
                                 nbins:int=100, xlim:Optional[Iterable]=None, title_label:str='events')->List[Figure]:
    all_figs = []
    
    calc_dict_sorted  = dict(sorted(calc_dict.items() , key=lambda key_val: np.sum(np.dot(key_val[1][0], key_val[1][1]))))
        
    plot_dict = {}
    plot_weights = []
    
    for key in calc_dict_sorted:
        plot_dict[key] = calc_dict_sorted[key][0]
        plot_weights.append(calc_dict_sorted[key][1])
    
    fig1, _, counts_flat = plot_hist(plot_dict, xlim=xlim, bins=nbins, custom_styling=True, stacked=True, weights=plot_weights, return_hist=True)
    fig2, _, counts_wt   = plot_hist(plot_dict, xlim=xlim, bins=nbins, custom_styling=True, stacked=True, return_hist=True)
    
    counts_flat = counts_flat[0]
    counts_wt   = counts_wt[0]
    
    counts_flat = counts_flat[counts_flat > 0]
    counts_wt   = counts_wt  [counts_wt > 0]
    
    yscale_flat = 'log' if (np.max(counts_flat)/np.min(counts_flat) > 100) else 'linear'
    yscale_wt   = 'log' if (np.max(  counts_wt)/np.min(counts_wt  ) > 100) else 'linear'
    
    if xlim is None:
        xlim = fig1.get_axes()[0].get_xlim()
    
    weighted = True
    for fig in [fig1, fig2]:
        fig = fig_ild_style(fig, xlim, nbins, xunit=xunit, xlabel=xlabel, yscale=yscale_wt if weighted else yscale_flat,
                    legend_labels=list(plot_dict.keys()), legend_kwargs={'loc': 'lower right'},
                    ylabel_prefix='wt. ' if weighted else '',
                    ild_offset_x=0.2, title=rf'ZHH → {hypothesis} analysis ('
                                                        + (r"$\bf{wt.}$ " if weighted else "")
                                                        + (rf"{title_label}")
                                                    + ')')
        
        all_figs.append(fig)
        
        weighted = False
        
    return all_figs

def plot_preselection_pass(vecs:np.ndarray) -> List:
    """_summary_

    Args:
        vecs (np.ndarray): _description_

    Returns:
        List: list of figures
    """
    
    import seaborn as sns
    import matplotlib.pyplot as plt
    from matplotlib.figure import Figure

    bars = list(range(len(vecs.T) + 1))
    vals = [len(vecs)]

    temp = np.ones(len(vecs), dtype=int)

    for i in range(len(vecs.T)):
        temp = temp & vecs.T[i]
        vals.append(np.sum(temp))
        
    del temp
    
    desc = [
        'All',
        'nJets',
        'nIsoLeps',
        'DiLepMDif',
        
        'DiJetMDif1',
        'DiJetMWdw1',
        'DiJetMDif2',
        'DiJetMWdw2',
        
        'MissPTWdw',
        'Thrust',
        'Evis',
        'MinHHMass',
        'nBJets',
    ]
    
    # Plot consecutive (&) cuts
    ax1 = sns.barplot(x=bars, y=vals, )
    
    ax1.set_yscale('log')
    ax1.set_ylabel('Number of events')
    ax1.set_xlabel('Preselection cut')
    ax1.set_title(rf'$2f$ events surviving cats (consecutive)')
    ax1.set_xticklabels(desc, rotation=45)
    
    # Plot individual cuts
    plt.show()
    
    ax2 = sns.barplot(x=bars, y=[ len(vecs), *[ np.sum(vecs.T[i]) for i in range(len(vecs.T)) ]])
    ax2.set_yscale('log')
    ax2.set_ylabel('Number of events')
    ax2.set_xlabel('Preselection cut')
    ax2.set_title(rf'$2f$ events surviving cats (individual)')
    ax2.set_xticklabels(desc, rotation=45)
    
    return [
        ax1.get_figure(),
        ax2.get_figure()
    ]