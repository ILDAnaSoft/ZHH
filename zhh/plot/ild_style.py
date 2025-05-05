from matplotlib.ticker import AutoMinorLocator, LogLocator, Locator, MultipleLocator
from matplotlib.patches import Patch
from matplotlib.figure import Figure
from matplotlib.axes import Axes
from matplotlib.legend_handler import HandlerTuple
from typing import Optional, List, Iterable, Union, Tuple, Literal
from ..util.get_matplotlib_fonts import resolve_fonts
from ..util.PlotContext import PlotContext

ild_style_defaults = {
    'fontname': ['Liberation Sans'],
    'beam_energy': 550,
    'ild_offset_x': 0.,
    'ild_offset_y': 0.,
    'ild_status': 'preliminary',
    'colorpalette': None,
    'luminosity_inv_ab': 2,
    'polarization_ep': 0.3,
    'polarization_em': -0.8
}

def fig_ild_style(fig_or_ax:Figure|Axes, xlim:Union[List[float], Tuple[float,float]], bins:Union[Iterable,int],
                  xscale:str='linear', xunit:Optional[str]='GeV', xlabel:str='m',
                  yscale:str='linear', yunit:Optional[str]='events', ylabel_prefix:str='',
                  xlocator:Optional[Locator]=None, ylocator:Optional[Locator]=None,
                  xminor:Optional[int]=5, yminor:Optional[int]=5,
                  fontname:str|list[str]=ild_style_defaults['fontname'],
                  beam_spec:bool=True, beam_energy:int=ild_style_defaults['beam_energy'], ax_index:int=0,
                  title:Optional[str]=None, title_postfix:str='',
                  legend_labels:Optional[List]=None, legend_kwargs={},
                  colorpalette:Optional[List]=ild_style_defaults['colorpalette'],
                  plot_context:PlotContext|None=None,
                  ild_text_position:Optional[Literal['upper left','upper right']]='upper left',
                  ild_offset_x:float=ild_style_defaults['ild_offset_x'],
                  ild_offset_y:float=ild_style_defaults['ild_offset_y'],
                  ild_status:str=ild_style_defaults['ild_status'],
                  show_binning_on_y_scale:bool=True)->Figure:
    
    if isinstance(fig_or_ax, Axes):
        fig = fig_or_ax.figure
        ax = fig_or_ax
    elif isinstance(fig_or_ax, Figure):
        fig = fig_or_ax
        ax = fig.get_axes()[ax_index]
    else:
        raise TypeError('fig_or_ax must be a Figure or Axes instance')
    
    use_facecolor = True
    if colorpalette is None:
        #from phc import get_colorpalette
        #colorpalette = get_colorpalette()
        n_patches = len(ax.patches)
        
        if plot_context is not None:
            if hasattr(fig, 'columns'):
                colorpalette = plot_context.getColorPalette(fig.columns)
            else:
                raise Exception('Could not get colors for legend. Assign the .columns attribute to the figure.')
        else:
            use_facecolor = ax.patches[0].get_facecolor() != (1,1,1,0)
            colorpalette = [getattr(ax.patches[(n_patches -1 - i) if use_facecolor else i], 'get_facecolor' if use_facecolor else 'get_edgecolor')() for i in range(n_patches)]
    
    if yunit is None or yunit =='':
        yunit = '1'
        
    if title is not None:
        if title_postfix != '':
            title = title[:-1] + title_postfix + ')'
            
        title = fancify_formula(title)
        
    if ild_offset_x == 0 and ild_offset_y == 0:
        if ild_text_position == 'upper left':
            ild_offset_x = 0.1
            ild_offset_y = 0.89
        elif ild_text_position == 'upper right':
            ild_offset_x = 0.7
            ild_offset_y = 0.89
    
    if isinstance(fontname, list):
        fontname = resolve_fonts(fontname)
    
    ax.text(ild_offset_x, ild_offset_y, f'ILD {ild_status}', fontsize=12, weight='bold', fontname=fontname, transform=ax.transAxes)
    
    if beam_spec:
        ax.text(ild_offset_x, ild_offset_y-.035, rf'$\sqrt{{s}} = {beam_energy}$ GeV, $L_{{int}} = {ild_style_defaults["luminosity_inv_ab"]}$ab$^{{-1}}$', fontsize=8, fontname=fontname, transform=ax.transAxes)
        ax.text(ild_offset_x, ild_offset_y-.065, rf'$P(e^{{+}}, e^{{-}}) = ({ild_style_defaults["polarization_ep"]:+.1}, {ild_style_defaults["polarization_em"]:+.1})$', fontsize=8, fontname=fontname, transform=ax.transAxes)
    
    for label in ax.get_xticklabels():
        label.set_fontname(fontname)
        
    for label in ax.get_yticklabels():
        label.set_fontname(fontname)
        
    if xscale =='linear':
        if xminor is not None:
            ax.xaxis.set_minor_locator(AutoMinorLocator(xminor))
    else:
        raise NotImplementedError('Log scale not yet implemented')
    
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
    
    nbins = len(bins) if isinstance(bins, Iterable) else bins
    
    y_label = ylabel_prefix + rf'{yunit}'
    if show_binning_on_y_scale:
        ylabel_binning_val = (xlim[1]-xlim[0])/nbins
        ylabel_binning_str = rf'{ylabel_binning_val:.2f}' if ylabel_binning_val >= 0.1 else rf'{ylabel_binning_val:.2E}'
        y_label += rf' / {ylabel_binning_str}'
        
        if (xunit != '' and xunit is not None):
            y_label += f' {xunit}'
    
    ax.set_xlabel(rf'${xlabel}$' + ('' if (xunit == '' or xunit is None) else f' [{xunit}]'), fontsize=12, fontname=fontname)
    ax.set_ylabel(y_label, fontsize=12, fontname=fontname)
    
    if title is not None:
        ax.set_title(title, loc='right', fontsize=12, fontname=fontname)
    
    if legend_labels is not None:
        legend_handles = []
        
        for i in reversed(range(len(legend_labels))):
            process_name = legend_labels[i]
            legend_handles.append([Patch(facecolor=colorpalette[i], edgecolor=colorpalette[i], label=process_name)])
            
        ax.legend(handles=legend_handles, labels=list(reversed(legend_labels)), fontsize=10, handler_map={list: HandlerTuple(ndivide=len(legend_labels), pad=0)}, **legend_kwargs)
        
    return fig

def fancify_formula(s:str):
    for (a, b) in [
        ('>=', ' \\geq '),
        ('<=', ' \\leq '),
    ]:
        s = s.replace(a, b)
    
    return s