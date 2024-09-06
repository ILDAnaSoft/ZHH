from typing import Optional, List, Tuple, Iterable
from tqdm.auto import tqdm
from .ild_style import fig_ild_style
from phc import plot_hist
import numpy as np
from matplotlib.figure import Figure

# For a given final state, plots the most important processes
def plot_preselection_by_event_category(presel_results:np.ndarray, processes:np.ndarray, weights:np.ndarray,
                                        hypothesis:str, event_categories:list=[17], quantity:str='ll_mz',
                                        unit:str='GeV', xlim:Optional[Iterable]=None, nbins:int=100, xlabel:Optional[str]=None,
                                        yscale:Optional[str]=None, ild_style_kwargs:dict={})->List[Figure]:
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

            mask_process = mask_process & mask
            mask_process_pass = mask_process & presel_results[f'{hypothesis_key}_pass']
            
            # Assumes indices of weights = weights['pid']
            calc_dict_all [process_name] = (presel_results[quantity][mask_process], weights['weight'][presel_results['pid'][mask_process]])
            calc_dict_pass[process_name] = (presel_results[quantity][mask_process_pass], weights['weight'][presel_results['pid'][mask_process_pass]])
        
        # Sort by number of entries
        figs_all  = plot_preselection_by_calc_dict(calc_dict_all , unit, hypothesis, xlabel, nbins=nbins, xlim=xlim, yscale=yscale, title_label=rf'events', ild_style_kwargs=ild_style_kwargs)
        figs_pass = plot_preselection_by_calc_dict(calc_dict_pass, unit, hypothesis, xlabel, nbins=nbins, xlim=xlim, yscale=yscale, title_label=rf'events passing {hypothesis}', ild_style_kwargs=ild_style_kwargs)
        
        all_figs += figs_all + figs_pass
        
    return all_figs

def plot_preselection_by_event_categories(presel_results:np.ndarray, processes:np.ndarray, weights:np.ndarray, category_map_inv:dict,
                                          hypothesis:str, event_categories:list=[11, 16, 12, 13, 14], additional_event_categories:Optional[int]=3,
                                        quantity:str='ll_mz', unit:str='GeV', xlabel:Optional[str]=None,
                                        nbins:int=100, xlim:Optional[tuple]=None,
                                        check_pass:bool=False, plot_flat:bool=True, yscale:Optional[str]=None,
                                        ild_style_kwargs:dict={})->Figure:
    
    """Plot the preselection results for a given hypothesis by event categories

    Args:
        presel_results (np.ndarray): _description_
        weights (np.ndarray): _description_
        hypothesis (str): _description_
        event_categories (list, optional): _description_. Defaults to [17].
        additional_event_categories (Optional[int], optional): _description_. Defaults to 3.
        quantity (str, optional): _description_. Defaults to 'll_mz'.
        unit (str, optional): _description_. Defaults to 'GeV'.
        xlabel (Optional[str], optional): _description_. Defaults to None.
        nbins (int, optional): _description_. Defaults to 100.
        xlim (Optional[tuple], optional): _description_. Defaults to None.
        check_pass (bool, optional): _description_. Defaults to False.
        yscale (Optional[str], optional): _description_. Defaults to None.
        ild_style_kwargs (dict, optional): _description_. Defaults to {}.

    Returns:
        _type_: _description_
    """
    
    if xlabel is None:
        xlabel = quantity
    
    hypothesis_key = hypothesis[:2]
    
    # Find relevant event categories    
    categories, counts = np.unique(presel_results['event_category'], return_counts=True)    
    count_sort_ind = np.argsort(-counts)
    
    categories = categories[count_sort_ind]
    #counts = counts[count_sort_ind]
    
    mask = np.isin(categories, event_categories)
    
    if additional_event_categories is not None:
        n_additional = 0
        for i in range(len(categories)):
            if not mask[i]:
                if n_additional < additional_event_categories:
                    mask[i] = True
                    n_additional += 1
                else:
                    break
                
    categories = categories[mask]
    #counts = counts[mask]
    
    calc_dict_all  = {}
    calc_dict_pass = {}
    
    for category in (pbar := tqdm(categories)):
        label = category_map_inv[category]
        pbar.set_description(f'Processing event category {label}')
        
        covered_processes = []

        mask = (presel_results['event_category'] == category)
        pids = np.unique(presel_results['pid'][mask])
        
        process_masks  = []
        
        # Collect contributions from all proc_pol combinations of a process
        for pid in pids:
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
            
            # Use that indices of weights = weights['pid']
            mask_process = mask & mask_process
            
            process_masks.append(mask_process)
        
        category_mask = np.logical_or.reduce(process_masks)
        
        calc_dict_all[label] = (presel_results[quantity][category_mask], weights['weight'][presel_results['pid'][category_mask]])
        
        if check_pass:
            category_mask_pass = category_mask & presel_results[f'{hypothesis_key}_pass']
            calc_dict_pass[label] = (presel_results[quantity][category_mask_pass], weights['weight'][presel_results['pid'][category_mask_pass]])
            
    all_figs = plot_preselection_by_calc_dict(calc_dict_all, unit, hypothesis, xlabel, nbins=nbins, xlim=xlim,
                                              title_label=rf'events', plot_flat=plot_flat, yscale=yscale,
                                              ild_style_kwargs=ild_style_kwargs)
    if check_pass:
        all_figs += plot_preselection_by_calc_dict(calc_dict_pass, unit, hypothesis, xlabel, nbins=nbins, xlim=xlim,
                                                   title_label=rf'events passing {hypothesis}', yscale=yscale,
                                                   ild_style_kwargs=ild_style_kwargs)
    
    return all_figs

def plot_preselection_by_calc_dict(calc_dict, xunit:str, hypothesis:str, xlabel:str,
                                 nbins:int=100, xlim:Optional[Iterable]=None, title_label:str='events',
                                 plot_flat:bool=True, yscale:Optional[str]=None, ild_style_kwargs:dict={})->List[Figure]:
    all_figs = []
    
    calc_dict_sorted = dict(sorted(calc_dict.items(), key=lambda key_val: np.sum(np.dot(key_val[1][0], key_val[1][1]))))
    
    plot_dict = {}
    plot_weights = []
    
    for key in calc_dict_sorted:
        plot_dict[key] = calc_dict_sorted[key][0]
        plot_weights.append(calc_dict_sorted[key][1])
    
    fig1, _, counts_wt   = plot_hist(plot_dict, xlim=xlim, bins=nbins, custom_styling=True, stacked=True, weights=plot_weights, return_hist=True)
    
    if plot_flat:
        fig2, _, counts_flat = plot_hist(plot_dict, xlim=xlim, bins=nbins, custom_styling=True, stacked=True, return_hist=True)
    
    if yscale is None:
        if plot_flat:
            counts_flat = counts_flat[0]
            counts_flat = counts_flat[counts_flat > 0]
            yscale_flat = 'log' if (np.max(counts_flat)/np.min(counts_flat) > 100) else 'linear'
            
        counts_wt = counts_wt[0]
        counts_wt = counts_wt[counts_wt > 0]
        yscale_wt = 'log' if (np.max(  counts_wt)/np.min(counts_wt  ) > 100) else 'linear'
    else:
        yscale_flat = yscale_wt = yscale
    
    if xlim is None:
        xlim = fig1.get_axes()[0].get_xlim()
    
    weighted = True
    for fig in ([fig1, fig2] if plot_flat else [fig1]):
        fig_ild_kwargs = {
            'xunit': xunit,
            'xlabel': xlabel,
            'yscale': yscale_wt if weighted else yscale_flat,
            'ild_offset_x': 0.,
            'ylabel_prefix': 'wt. ' if weighted else '',
            'title_postfix': '',
            'title': rf'ZHH → {hypothesis} analysis ('+ (r"$\bf{wt.}$ " if weighted else "") + (rf"{title_label}") + ')'
        } | ild_style_kwargs
        
        fig = fig_ild_style(fig, xlim, nbins, legend_labels=list(plot_dict.keys()), legend_kwargs={'loc': 'lower right'}, **fig_ild_kwargs)
        
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