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

def plot_preselection_by_calc_dict(calc_dict, hypothesis:str, xlabel:str, plot_context:PlotContext,
                                   xunit:Optional[str]=None,
                                 bins:int=100, xlim:Optional[Iterable]=None, title_label:str='events',
                                 yscale:Optional[str]=None,
                                 ild_style_kwargs:dict={},
                                 plot_hist_kwargs:dict={},
                                 plot_hist_kwargs_overwrite:dict={},
                                 ax:Optional[Axes]=None):
    
    from phc import plot_hist
    
    plot_dict = {}
    plot_weights = []
    
    for key in calc_dict:
        plot_dict[key] = calc_dict[key][0]
        plot_weights.append(calc_dict[key][1])
        
    fig_plot_hist_kwargs = {
        'stacked': True,
        'custom_styling': True,
        'colorpalette': None if plot_context is None else plot_context.getColorPalette(list(plot_dict.keys()))
    }
    
    fig_plot_hist_kwargs = fig_plot_hist_kwargs | plot_hist_kwargs
    
    if ax is not None:
        fig_plot_hist_kwargs['ax'] = ax
    
    fig, _, counts_wt = plot_hist(plot_dict, xlim=xlim, bins=bins, weights=plot_weights, return_hist=True,
                                  hist_kwargs_overwrite=plot_hist_kwargs_overwrite, **fig_plot_hist_kwargs)
    
    if ax is None:
        ax = fig.get_axes()[0]
    
    if yscale is None:            
        counts_wt = counts_wt[0]
        counts_wt = counts_wt[counts_wt > 0]
        yscale_wt = 'log' if (np.max(counts_wt)/np.min(counts_wt) > 100) else 'linear'
    else:
        yscale_wt = yscale
    
    if xlim is None:
        xlim = ax.get_xlim()
    
    fig_ild_kwargs = {
        'xunit': xunit,
        'xlabel': xlabel,
        'yscale': yscale_wt,
        'ild_offset_x': 0.,
        'ylabel_prefix': 'wt. ',
        'title_postfix': '',
        'title': rf'ZHH → {hypothesis} analysis ('+ (r"wt. ") + (rf"{title_label}") + ')',
        'legend_kwargs': {'loc': 'lower right'}
    } | ild_style_kwargs
    
    legend_labels = []
    for key, value in plot_dict.items():
        if len(value) > 0:
            legend_labels.append(key)
            
    fig = fig_ild_style(ax, xlim, bins, legend_labels=legend_labels, colorpalette=list(map(plot_context.getColorByKey, plot_dict.keys())), **fig_ild_kwargs)
    
    return fig

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