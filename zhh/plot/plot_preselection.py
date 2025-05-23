from typing import Optional, List, Tuple, Iterable
from collections.abc import Sequence
from tqdm.auto import tqdm
from .ild_style import fig_ild_style
from ..analysis.PreselectionAnalysis import calc_preselection_by_event_categories
import numpy as np
import math
import matplotlib.pyplot as plt
from matplotlib.figure import Figure
from matplotlib.axes import Axes
from ..analysis.Cuts import Cut, EqualCut, WindowCut, GreaterThanEqualCut, LessThanEqualCut
from ..analysis.ZHHCuts import zhh_cuts
from ..util.PlotContext import PlotContext

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
        figs_all  = plot_preselection_by_calc_dict(calc_dict_all , hypothesis, xlabel, unit, nbins=nbins, xlim=xlim, yscale=yscale, title_label=rf'events', ild_style_kwargs=ild_style_kwargs)
        figs_pass = plot_preselection_by_calc_dict(calc_dict_pass, hypothesis, xlabel, unit, nbins=nbins, xlim=xlim, yscale=yscale, title_label=rf'events passing {hypothesis}', ild_style_kwargs=ild_style_kwargs)
        
        all_figs += figs_all + figs_pass
        
    return all_figs

def plot_preselection_by_event_categories(presel_results:np.ndarray, processes:np.ndarray, weights:np.ndarray,
                                          hypothesis:str, quantity:str, weighted:bool=True,
                                          categories_selected:list=[11, 16, 12, 13, 14],
                                          categories_additional:Optional[int]=3,
                                        unit:str='GeV', xlabel:Optional[str]=None,
                                        bins:int=100, xlim:Optional[tuple]=None,
                                        plot_flat:bool=True, yscale:Optional[str]=None,
                                        ild_style_kwargs:dict={})->List[Figure]:
    
    if xlabel is None:
        xlabel = quantity
        
    calc_dics = calc_preselection_by_event_categories(
        presel_results, processes, weights,
        quantity=quantity,
        categories_selected=categories_selected,
        categories_additional=categories_additional,
        xlim=xlim)
    
    return plot_preselection_by_calc_dict(calc_dics[0],
                                              hypothesis=hypothesis,
                                              xlabel=xlabel,
                                              xunit=unit, bins=bins, xlim=xlim,
                                              title_label=rf'events', plot_flat=plot_flat, yscale=yscale,
                                              ild_style_kwargs=ild_style_kwargs)

def plot_preselection_by_calc_dict(calc_dict, hypothesis:str, xlabel:str, xunit:Optional[str]=None,
                                 bins:int=100, xlim:Optional[Iterable]=None, title_label:str='events',
                                 plot_flat:bool=False, yscale:Optional[str]=None,
                                 ild_style_kwargs:dict={}, plot_hist_kwargs:dict={},
                                 plot_context:Optional[PlotContext]=None)->List[Figure]:
    
    from phc import plot_hist
    
    all_figs = []
    
    plot_dict = {}
    plot_weights = []
    
    for key in calc_dict:
        plot_dict[key] = calc_dict[key][0]
        plot_weights.append(calc_dict[key][1])
        
    fig_plot_hist_kwargs = {
        'stacked': True,
        'colorpalette': None if plot_context is None else plot_context.getColorPalette(list(plot_dict.keys()))
    } | plot_hist_kwargs
    
    fig1, _, counts_wt = plot_hist(plot_dict, xlim=xlim, bins=bins, custom_styling=True, weights=plot_weights, return_hist=True, **fig_plot_hist_kwargs)
    
    if plot_flat:
        fig2, _, counts_flat = plot_hist(plot_dict, xlim=xlim, bins=bins, custom_styling=True, return_hist=True, **fig_plot_hist_kwargs)
    
    if yscale is None:
        if plot_flat:
            counts_flat = counts_flat[0]
            counts_flat = counts_flat[counts_flat > 0]
            yscale_flat = 'log' if (np.max(counts_flat)/np.min(counts_flat) > 100) else 'linear'
            
        counts_wt = counts_wt[0]
        counts_wt = counts_wt[counts_wt > 0]
        yscale_wt = 'log' if (np.max(counts_wt)/np.min(counts_wt) > 100) else 'linear'
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
            'title': rf'ZHH → {hypothesis} analysis ('+ (r"wt. " if weighted else "") + (rf"{title_label}") + ')',
            'legend_kwargs': {'loc': 'lower right'}
        } | ild_style_kwargs
        
        legend_labels = []
        for key, value in plot_dict.items():
            if len(value) > 0:
                legend_labels.append(key)
                
        fig = fig_ild_style(fig, xlim, bins, legend_labels=legend_labels, **fig_ild_kwargs)
        
        all_figs.append(fig)
        
        weighted = False
        
    return all_figs

def annotate_cut(ax:Axes, cut:Cut, spancolor:str='red', alpha:float=0.7):
    if isinstance(cut, WindowCut):
        ax.axvspan(cut.lower, cut.upper, .97,  color=spancolor, alpha=alpha, hatch='*')
        ax.axvline(x=cut.lower, linewidth=1.5, color=spancolor)
        ax.axvline(x=cut.upper, linewidth=1.5, color=spancolor)
        
    else:
        if isinstance(cut, GreaterThanEqualCut) or isinstance(cut, LessThanEqualCut):
            value = cut.lower if isinstance(cut, GreaterThanEqualCut) else cut.upper
            xlim = ax.get_xlim()
            
            if isinstance(value, int):
                offset = 0.5 if isinstance(cut, GreaterThanEqualCut) else -0.5
            else:
                offset = 0
                
            xlimspan = (value, xlim[1] + offset) if isinstance(cut, GreaterThanEqualCut) else (xlim[0] + offset, value)            
            
            ax.axvspan(xlimspan[0], xlimspan[1], .97, alpha=alpha, color=spancolor, hatch='*')
            ax.axvline(x=xlimspan[0 if isinstance(cut, GreaterThanEqualCut) else 1] - offset, linewidth=1.5, color=spancolor)
        elif isinstance(cut, EqualCut):
            value = cut.value
        else:
            raise ValueError(f'Unknown cut type {type(cut)}')
        
        if isinstance(value, int):
            ax.axvspan(value - .5, value + .5, .97, alpha=alpha, color=spancolor, hatch='*')
        else:
            ax.axvline(x=value, linewidth=1.5, color=spancolor)
            

def plot_total_efficiency(counts_first:dict, counts_last:dict, hypothesis:str,
                          signal_categories:List[str],
                          colorpalette:Optional[List[str]]=None,
                          color_dict:Optional[dict]=None):
    
    if color_dict is not None:
        colorpalette = [color_dict[key] for key in signal_categories]
        
    if colorpalette is None:
        from phc import get_colorpalette
        colorpalette = get_colorpalette()
    
    labels = np.array(signal_categories)
    ratios = np.array([counts_last[key]/counts_first[key] for key in labels])
    order = np.argsort(-ratios)
    
    fig, ax = plt.subplots(figsize=(8, 4))
    ax.bar(labels[order], ratios[order], color=colorpalette)
    ax.legend()
    
    ax.set_title(f'Totel efficiency for {hypothesis}: {sum([counts_last[key] for key in labels])/sum([counts_first[key] for key in labels]):.1%}')
    ax.set_ylabel('Efficiency after pre-cuts')
    ax.bar_label(ax.containers[0], labels=[f'{r:.1%}' for r in ratios[order]])
    
    return fig

def plot_cut_efficiencies(counts_all:List[dict], hypothesis:str, signal_categories:List[str],
                          colorpalette:Optional[list[str]]=None,
                          plot_context:Optional[PlotContext]=None,
                          cuts:Optional[Sequence[Cut]]=None,
                          width:float = 0.6, spacing:float=0.2):
    
    if colorpalette is None:
        if plot_context is not None:
            colorpalette = plot_context.getColorPalette(signal_categories)
        else:
            from phc import get_colorpalette
            colorpalette = get_colorpalette()
        
    if cuts is None:
        cuts = zhh_cuts(hypothesis)
    
    figs = []
    ncats = len(signal_categories)
    
    fig, ax = plt.subplots(figsize=(5*ncats, 6))
    X_axis = (width*ncats + spacing)*np.arange(len(cuts))
    
    ax.set_title(f'Efficiency of preselection cuts ({hypothesis})')
    #ax.set_yscale('log')
    
    X_offsets = np.zeros(ncats) if ncats == 1 else (-(width/2 if (ncats % 2 != 0) else 0) + width * (
        (ncats/2) - np.arange(ncats)
    ))
    
    for i, key in enumerate(signal_categories):
        ratios = []
        labels = []
        
        for j, cut in enumerate(cuts):
            ratio = counts_all[j+1][key] / counts_all[j][key]
            ratios.append(0 if math.isnan(ratio) else ratio)
            labels.append(rf'${cut.label}$')
        print(ratios)
        ax.bar(X_axis + X_offsets[i], ratios, width, color=colorpalette[i], align='center', label=key)
        ax.bar_label(ax.containers[i], labels=[f'{r:.1%}' for r in ratios])
        
    ax.set_xticks(X_axis, labels)
    ax.legend(loc='lower right')
    
    figs.append(fig)
    
    return figs