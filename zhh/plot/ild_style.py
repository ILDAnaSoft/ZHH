from matplotlib.ticker import AutoMinorLocator, LogLocator, Locator, MultipleLocator
from matplotlib.patches import Patch
from matplotlib.figure import Figure
from matplotlib.axes import Axes
from matplotlib.legend_handler import HandlerTuple
from collections.abc import Collection
from typing import Optional, List, Literal
from ..util.get_matplotlib_fonts import resolve_fonts
from ..util.PlotContext import PlotContext
from ..util.deepmerge import deepmerge
from copy import deepcopy

ild_style_defaults = {
    'fontname': ['DejaVu Sans'],
    'beam_energy': 550,
    'ild_offset_x': 0.,
    'ild_offset_y': 0.,
    'ild_status': 'preliminary',
    'colorpalette': None,
    'luminosity_inv_ab': 2,
    'polarization_ep': 0.3,
    'polarization_em': -0.8
}

def fig_ild_style(fig_or_ax:Figure|Axes, xlim:list[float]|tuple[float,float]|None=None, bins:Collection|int|None=None,
                  xscale:str='linear', xunit:Optional[str]='GeV', xlabel:str='m',
                  yscale:str='linear', yunit:Optional[str]='events', ylabel_prefix:str='',
                  xlocator:Optional[Locator]=None, ylocator:Optional[Locator]=None,
                  xminor:Optional[int]=5, yminor:Optional[int]=5,
                  beam_spec:bool=True, beam_energy:int=ild_style_defaults['beam_energy'], ax_index:int=0,
                  title:Optional[str]=None, title_postfix:str='',
                  legend_labels:Optional[List]=None,
                  legend_kwargs={},
                  colorpalette:Optional[List]=ild_style_defaults['colorpalette'],
                  plot_context:PlotContext|None=None, int_bins:bool=False,
                  ild_text_position:Optional[Literal['upper left','upper right']]='upper left',
                  ild_offset_x:float=ild_style_defaults['ild_offset_x'],
                  ild_offset_y:float=ild_style_defaults['ild_offset_y'],
                  ild_status:str=ild_style_defaults['ild_status'],
                  show_binning_on_y_scale:bool|None=None,
                  columns:list[str]|None=None)->Figure:
    
    if isinstance(fig_or_ax, Axes):
        fig = fig_or_ax.figure
        ax = fig_or_ax
    elif isinstance(fig_or_ax, Figure):
        fig = fig_or_ax
        ax = fig.get_axes()[ax_index]
    else:
        raise TypeError('fig_or_ax must be a Figure or Axes instance')
    
    if show_binning_on_y_scale is None:
        show_binning_on_y_scale = int_bins
    
    use_facecolor = True
    is_hist = len(ax.patches) > 0
    
    if is_hist and colorpalette is None:
        #from phc import get_colorpalette
        #colorpalette = get_colorpalette()
        n_patches = len(ax.patches)
        
        if plot_context is not None:
            if hasattr(fig, 'columns') and isinstance(getattr(fig, 'columns'), list):
                colorpalette = plot_context.getColorPalette(getattr(fig, 'columns'))
            elif columns is not None:
                colorpalette = plot_context.getColorPalette(columns)
            else:
                colorpalette = plot_context.getColorPalette(plot_context.getColorsAssignedKeys())
                print('Could not get colors for legend. Using all properties of PlotContext')
                #raise Exception('Could not get colors for legend. Assign the .columns attribute to the figure.')
        else:
            use_facecolor = ax.patches[0].get_facecolor() != (1,1,1,0)
            colorpalette = [getattr(ax.patches[(n_patches -1 - i) if use_facecolor else i], 'get_facecolor' if use_facecolor else 'get_edgecolor')() for i in range(n_patches)]
    
    if title is not None:
        if title_postfix != '':
            title = title[:-1] + title_postfix + ')'
            
        title = fancify_formula(title)
        
    if ild_offset_x == 0 and ild_offset_y == 0:
        if ild_text_position == 'upper left':
            ild_offset_x = 0.04
            ild_offset_y = 0.92
            
            #ild_offset_x = 0.1
            #ild_offset_y = 0.89
        elif ild_text_position == 'upper right':
            ild_offset_x = 0.7
            ild_offset_y = 0.89
    
    fontname = plot_context.getFont() if plot_context is not None else resolve_fonts(ild_style_defaults['fontname'])
    
    ax.text(ild_offset_x, ild_offset_y, f'ILD {ild_status}', fontsize=12, weight='bold', fontname=fontname, transform=ax.transAxes)
    
    if beam_spec:
        ax.text(ild_offset_x, ild_offset_y-.035, rf'$\sqrt{{s}} = {beam_energy}$ GeV, $L_{{int}} = {ild_style_defaults["luminosity_inv_ab"]}$ab$^{{-1}}$', fontsize=8, fontname=fontname, transform=ax.transAxes)
        ax.text(ild_offset_x, ild_offset_y-.065, rf'$P(e^{{+}}, e^{{-}}) = ({ild_style_defaults["polarization_ep"]:+.1}, {ild_style_defaults["polarization_em"]:+.1})$', fontsize=8, fontname=fontname, transform=ax.transAxes)
        
    if xscale =='linear':
        if xminor is not None:
            ax.xaxis.set_minor_locator(AutoMinorLocator(xminor))
    else:
        if is_hist:
            raise NotImplementedError('Log x-scale not supported for histograms')
        else:
            ax.set_xscale('log')
            ax.xaxis.set_major_locator(LogLocator(base=10))
            ax.xaxis.set_minor_locator(LogLocator(base=10,subs=[2., 5., 6., 7., 8., 9.]))
    
    if yscale =='log':
        ax.set_yscale('log')
        ax.yaxis.set_major_locator(LogLocator(base=10))
        ax.yaxis.set_minor_locator(LogLocator(base=10,subs=[2., 5., 6., 7., 8., 9.]))
    else:
        if yminor is not None:
            ax.yaxis.set_minor_locator(AutoMinorLocator(yminor))        
    
    if xlocator is not None:
        ax.xaxis.set_major_locator(xlocator)
    
    if ylocator is not None:
        ax.yaxis.set_major_locator(ylocator)
    
    ax.xaxis.set_ticks_position('both')
    ax.yaxis.set_ticks_position('both')
    
    ax.tick_params(axis='both', width=1.5, length=5, which='both', direction='in', labelsize=12)
    ax.tick_params(axis='both', width=2.5, length=8)
    
    y_label = ylabel_prefix + (rf'{yunit}' if (yunit is not None and yunit != '') else '')
    if is_hist and show_binning_on_y_scale:
        assert(xlim is not None and bins is not None)
        
        nbins = len(bins) if isinstance(bins, Collection) else bins
        ylabel_binning_val = (xlim[1]-xlim[0])/nbins
        ylabel_binning_str = rf'{ylabel_binning_val:.2f}' if ylabel_binning_val >= 0.1 else rf'{ylabel_binning_val:.2E}'
        y_label += rf' / {ylabel_binning_str}'
        
        if (xunit != '' and xunit is not None):
            y_label += f' {xunit}'
            
    x_label = rf'${xlabel}$ [{xunit}]' if (xunit is not None and xunit != '') else xlabel
    
    if is_hist and legend_labels is not None:
        assert(isinstance(colorpalette, list))
        
        legend_handles = []
        
        for i in reversed(range(len(legend_labels))):
            process_name = legend_labels[i]
            legend_handles.append(Patch(color=colorpalette[i], linewidth=0, label=process_name))
        
        if plot_context is not None:
            legend_call_kwargs = deepmerge(legend_kwargs_fn(plot_context), deepcopy(legend_kwargs))
        else:
            legend_call_kwargs = deepcopy(legend_kwargs)
        
        ax.legend(handles=legend_handles, **legend_call_kwargs)
    
    update_plot(ax, x_label=x_label, y_label=y_label, title=title, context=plot_context)
    
    return fig

def update_plot(ax:Axes, x_label:str|None=None, y_label:str|None=None, title:str|None=None, fontname:str|None=None, context:PlotContext|None=None,
                ):    
    
    if fontname is None:
        fontname = context.getFont() if context is not None else ild_style_defaults['fontname']
        
    assert(fontname is not None)
    
    for label in ax.get_xticklabels():
        label.set_fontname(fontname)
        
    for label in ax.get_yticklabels():
        label.set_fontname(fontname)
        
    x_label = x_label if x_label is not None else ax.get_xlabel()
    x_label = x_label if x_label != '' else None
        
    if x_label is not None:
        ax.set_xlabel(x_label, fontsize=12, fontname=fontname)
        
    y_label = y_label if y_label is not None else ax.get_ylabel()
    y_label = y_label if y_label != '' else None
        
    if y_label is not None:
        ax.set_ylabel(y_label, fontsize=12, fontname=fontname)
    
    title = title if title is not None else ax.get_title()
    title = title if title != '' else None
    
    if title is not None:
        ax.set_title(title, loc='right', fontsize=12, fontname=fontname)

def legend_kwargs_fn(context:PlotContext|None=None, size:int=9, titlesize:int=10):
    fontname = context.getFont() if context is not None else ild_style_defaults['fontname']
    
    return {
        'columnspacing': 0,
        'fancybox': False,
        'prop': {
            'family': fontname,
            'size': size,
        },
        'title_fontproperties': {'family': fontname, 'size': titlesize },
    }

def fancify_formula(s:str):
    for (a, b) in [
        ('>=', ' \\geq '),
        ('<=', ' \\leq '),
    ]:
        s = s.replace(a, b)
    
    return s