# Snapshot of plot_preselection_by_calc_dict
from typing import Optional, Iterable
from ..util.PlotContext import PlotContext
from .ild_style import fig_ild_style
import numpy as np
from matplotlib.axes import Axes

def plot_weighted_hist(calc_dict, title:str='<title undefined>', xlabel:str='<xlabel undefined>', plot_context:PlotContext|None=None,
                                   xunit:Optional[str]=None,
                                 bins:int=100, xlim:Optional[Iterable]=None,
                                 yscale:Optional[str]=None,
                                 ild_style_kwargs:dict={},
                                 plot_hist_kwargs:dict={},
                                 plot_hist_kwargs_overwrite:dict={},
                                 ax:Optional[Axes]=None):
    
    from phc import plot_hist
    if plot_context is None:
        plot_context = PlotContext()
    
    plot_dict = {}
    plot_weights = []
    
    for key in calc_dict:
        if len(calc_dict[key]) != 2:
            raise Exception(f'Invalid data format: Expected (data:[], weight:[]) for entry {key}')
        
        data = calc_dict[key][0]
        weight = calc_dict[key][1] if calc_dict[key][1] is not None else np.ones_like(data)
        
        plot_dict[key] = data
        plot_weights.append(weight)
        
    fig_plot_hist_kwargs = {
        'show_stats': False,
        #'normalize': True,
        'hist_kwargs': { 'hatch': None },
        'stacked': True,
        #'custom_styling': True,
        'colorpalette': plot_context.getColorPalette(list(plot_dict.keys()))
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
        'title': title,
        'legend_kwargs': { 'loc': 'upper right', 'bbox_to_anchor': (.98, .98), 'fancybox': False },
        'plot_context': plot_context
    } | ild_style_kwargs
    
    legend_labels = []
    for key, value in plot_dict.items():
        if len(value) > 0:
            legend_labels.append(key)
            
    fig = fig_ild_style(ax, xlim, bins, legend_labels=legend_labels, **fig_ild_kwargs)
    
    return fig