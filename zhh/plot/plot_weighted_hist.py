# Snapshot of plot_preselection_by_calc_dict
from ..util.PlotContext import PlotContext
from .ild_style import fig_ild_style
import numpy as np
from matplotlib.axes import Axes
from matplotlib.figure import Figure
from copy import deepcopy

def plot_combined_hist(calc_dict:dict[str, tuple[np.ndarray, np.ndarray]],
                       xlabel:str, plot_context:PlotContext,
                       hypothesis:str|None=None, xunit:str|None=None,
                       bins:int=100, xlim:tuple[float, float]|None=None, title_label:str='wt. events',
                       yscale:str|None=None,
                       ild_style_kwargs:dict={},
                       plot_hist_kwargs:dict={},
                       plot_hist_kwargs_overwrite:dict={},
                       ax:Axes|None=None,
                       signal_keys:list[str]=[])->Figure:
    """Plots the properties in calc_dict

    Args:
        calc_dict (dict[str, tuple[np.ndarray, np.ndarray]]): _description_
        xlabel (str): _description_
        plot_context (PlotContext): _description_
        hypothesis (str | None, optional): _description_. Defaults to None.
        xunit (str | None, optional): _description_. Defaults to None.
        bins (int, optional): _description_. Defaults to 100.
        xlim (Iterable | None, optional): _description_. Defaults to None.
        title_label (str, optional): _description_. Defaults to 'wt. events'.
        yscale (str | None, optional): _description_. Defaults to None.
        ild_style_kwargs (dict, optional): _description_. Defaults to {}.
        plot_hist_kwargs (dict, optional): _description_. Defaults to {}.
        plot_hist_kwargs_overwrite (dict, optional): _description_. Defaults to {}.
        ax (Axes | None, optional): _description_. Defaults to None.
        signal_keys (list[str], optional): _description_. Defaults to [].

    Returns:
        Figure: _description_
    """
    
    from phc import plot_hist
    from zhh import deepmerge
    from matplotlib.lines import Line2D
    
    calc_dict_mod:dict[str, tuple[np.ndarray, np.ndarray]] = {}
    sig_calc_dict:dict[str, tuple[np.ndarray, np.ndarray]] = {}
    
    for prop, val in calc_dict.items():
        if prop not in signal_keys:
            calc_dict_mod[prop] = val
        else:
            sig_calc_dict[prop] = val
    
    plot_dict = {}
    plot_weights = []
    
    for key in calc_dict_mod:
        plot_dict[key] = calc_dict[key][0]
        plot_weights.append(calc_dict[key][1])
    
    colorpalette = plot_context.getColorPalette(list(plot_dict.keys()))
    
    fig_plot_hist_kwargs = {
        'stacked': True,
        'custom_styling': True,
        'colorpalette': colorpalette,
        'hist_kwargs': { }
    }
    
    fig_plot_hist_kwargs = deepmerge(fig_plot_hist_kwargs, plot_hist_kwargs)
    
    if ax is not None:
        fig_plot_hist_kwargs['ax'] = ax
    
    if fig_plot_hist_kwargs['stacked'] and not 'label' in fig_plot_hist_kwargs['hist_kwargs']:
        fig_plot_hist_kwargs['hist_kwargs']['label'] = list(plot_dict.keys())
    
    fig_and_hist = plot_hist(plot_dict, xlim=xlim, bins=bins, weights=plot_weights, return_hist=True,
                                  hist_kwargs_overwrite=plot_hist_kwargs_overwrite, **fig_plot_hist_kwargs)
    
    assert(isinstance(fig_and_hist, tuple))
    fig, _, counts_wt = fig_and_hist
    
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
        'legend_kwargs': {'loc': 'lower right'},
        'int_bins': plot_hist_kwargs['int_bins'] if 'int_bins' in plot_hist_kwargs else False,
        'plot_context': plot_context,
        'colorpalette': colorpalette,
        'return_legend_kwargs': True
    }
    fig_ild_kwargs = deepmerge(fig_ild_kwargs, ild_style_kwargs)
    int_bins = fig_ild_kwargs['int_bins']
    
    if hypothesis is not None:
        fig_ild_kwargs['title'] = rf'ZHH{f" → {hypothesis}" if hypothesis is not None else ""} analysis' + (f' ({title_label})' if title_label is not None else '')
    
    legend_labels = []
    for key, value in plot_dict.items():
        if len(value) > 0:
            legend_labels.append(key)
            
    fig_and_kwags = fig_ild_style(ax, xlim, bins, legend_labels=legend_labels, **fig_ild_kwargs)
    assert(isinstance(fig_and_kwags, tuple))
    
    fig, legend_kwargs = fig_and_kwags
    
    # apply the sig
    ax = fig.axes[0]
    
    for sig_key, values_and_weights in sig_calc_dict.items():
        if int_bins:
            # bins already adjusted to -/+0.5
            sig_bins = xlim[0] + np.arange(round(xlim[1] - xlim[0]) + 2)
        else:
            sig_bins = fig_and_hist[1][0]
        
        h_sig = ax.hist(values_and_weights[0], weights=values_and_weights[1], color=plot_context.getColorByKey(sig_key),
                        bins=sig_bins, histtype='step', linewidth=3, label=sig_key)

        #handles, labels = ax.get_legend_handles_labels()
        #handles.append(Line2D([0], [0], color=plot_context.getColorByKey(sig_key),)) #edgecolor='black'))
        #labels.append(sig_key)
        #ax.legend(handles, labels, **legend_kwargs)

    ax.legend(**legend_kwargs)
    
    return fig

def plot_weighted_hist(calc_dict, title:str='<title undefined>', xlabel:str='<xlabel undefined>', plot_context:PlotContext|None=None,
                                xunit:str|None=None,
                                bins:int=100, xlim:tuple[float, float]|None=None,
                                yscale:str|None=None,
                                ild_style_kwargs:dict={},
                                plot_hist_kwargs:dict={},
                                plot_hist_kwargs_overwrite:dict={},
                                ax:Axes|None=None):
    
    from zhh import deepmerge
    
    if plot_context is None:
        from zhh import colormap_desy
        plot_context = PlotContext(colormap_desy)
        
    if title is not None:
        ild_style_kwargs['title'] = title
        
    fig_plot_hist_kwargs = {
        'show_stats': False,
        'hist_kwargs': { 'hatch': None },
        'stacked': False,
        'colorpalette': plot_context.getColorPalette(list(calc_dict.keys()))
    }
    fig_plot_hist_kwargs = deepmerge(fig_plot_hist_kwargs, deepcopy(plot_hist_kwargs))
    
    if ax is not None:
        plot_hist_kwargs['ax'] = ax
        
    fig_ild_kwargs = {
        'xunit': xunit,
        'xlabel': xlabel,
        'ild_offset_x': 0.,
        'title_postfix': '',
        'title': title,
        'legend_kwargs': { 'loc': 'upper right', 'bbox_to_anchor': (.98, .98), 'fancybox': False },
        'plot_context': plot_context,
        'columns': list(calc_dict.keys())
    }
    fig_ild_kwargs = deepmerge(fig_ild_kwargs, deepcopy(ild_style_kwargs))
    
    return plot_combined_hist(calc_dict, xlabel=xlabel, plot_context=plot_context,
                            hypothesis=None, xunit=xunit, bins=bins, xlim=xlim,
                            ild_style_kwargs=fig_ild_kwargs,
                            plot_hist_kwargs=fig_plot_hist_kwargs,
                            plot_hist_kwargs_overwrite=plot_hist_kwargs_overwrite,
                            yscale=yscale)

def plot_weighted_hist_old(calc_dict, title:str='<title undefined>', xlabel:str='<xlabel undefined>', plot_context:PlotContext|None=None,
                                   xunit:str|None=None,
                                 bins:int=100, xlim:tuple[float, float]|None=None,
                                 yscale:str|None=None,
                                 ild_style_kwargs:dict={},
                                 plot_hist_kwargs:dict={},
                                 plot_hist_kwargs_overwrite:dict={},
                                 ax:Axes|None=None):
    
    from phc import plot_hist
    if plot_context is None:
        plot_context = PlotContext()
    
    plot_dict = {}
    plot_weights = []
    
    is_weighted = True
    
    for key in calc_dict:
        if len(calc_dict[key]) != 2:
            raise Exception(f'Invalid data format: Expected (data:[], weight:[]) for entry {key}')
        
        data = calc_dict[key][0]
        if calc_dict[key][1] is not None:
            weight = calc_dict[key][1]
            
            if not is_weighted:
                raise Exception(f'Inconsistent data format: Entry {key} has weights, but previous entries did not.')
        else:            
            weight = np.ones_like(data)
            is_weighted = False
        
        plot_dict[key] = data
        plot_weights.append(weight)
        
    fig_plot_hist_kwargs = {
        'show_stats': False,
        #'normalize': True,
        'hist_kwargs': { 'hatch': None },
        'stacked': False,
        #'custom_styling': True,
        'colorpalette': plot_context.getColorPalette(list(plot_dict.keys()))
    }
    
    fig_plot_hist_kwargs = fig_plot_hist_kwargs | plot_hist_kwargs
    
    if ax is not None:
        fig_plot_hist_kwargs['ax'] = ax
    
    fig_and_hist = plot_hist(plot_dict, xlim=xlim, bins=bins, weights=plot_weights, return_hist=True,
                                  hist_kwargs_overwrite=plot_hist_kwargs_overwrite, **fig_plot_hist_kwargs)
    
    assert(isinstance(fig_and_hist, tuple))
    fig, _, counts_wt = fig_and_hist
    
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
        'ylabel_prefix': 'wt. ' if is_weighted else '',
        'title_postfix': '',
        'title': title,
        'legend_kwargs': { 'loc': 'upper right', 'bbox_to_anchor': (.98, .98), 'fancybox': False },
        'plot_context': plot_context,
        'columns': list(calc_dict.keys())
    } | ild_style_kwargs
    
    legend_labels = []
    for key, value in plot_dict.items():
        if len(value) > 0:
            legend_labels.append(key)
            
    fig = fig_ild_style(ax, xlim, bins, legend_labels=legend_labels, **fig_ild_kwargs)
    
    return fig