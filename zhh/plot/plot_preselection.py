from typing import Optional, List, Tuple, Iterable, cast
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

def plot_preselection_by_calc_dict(*args, **kwargs):
    """TO BE DEPRECATED. USE plot_combined_hist instead 

    Returns:
        _type_: _description_
    """
    from zhh import plot_combined_hist
    return plot_combined_hist(*args, **kwargs)

def annotate_cut(ax:Axes, cut:Cut, spancolor:str='red', alpha:float=0.7):
    from zhh import CutTypes
    
    if cut.type == CutTypes.CUT_WINDOW:
        cut = cast(WindowCut, cut)
        
        ax.axvspan(cut.lower, cut.upper, .97,  color=spancolor, alpha=alpha, hatch='*')
        ax.axvline(x=cut.lower, linewidth=1.5, color=spancolor)
        ax.axvline(x=cut.upper, linewidth=1.5, color=spancolor)
    else:
        if cut.type in [CutTypes.CUT_GTE, CutTypes.CUT_LTE]:
            is_gte = cut.type == CutTypes.CUT_GTE
            value = None
            if is_gte:
                cut = cast(GreaterThanEqualCut, cut)
                value = cut.lower
            else:
                cut = cast(LessThanEqualCut, cut)
                value = cut.upper
            
            assert(value is not None)
            
            xlim = ax.get_xlim()
            
            if isinstance(value, int):
                offset = 0.5 if is_gte else -0.5
            else:
                offset = 0
                
            xlimspan = (value, xlim[1] + offset) if is_gte else (xlim[0] + offset, value)            
            
            ax.axvspan(xlimspan[0], xlimspan[1], .97, alpha=alpha, color=spancolor, hatch='*')
            ax.axvline(x=xlimspan[0 if is_gte else 1] - offset, linewidth=1.5, color=spancolor)
        elif cut.type == CutTypes.CUT_EQ:
            cut = cast(EqualCut, cut)
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