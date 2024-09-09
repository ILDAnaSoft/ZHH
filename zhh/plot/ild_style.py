from matplotlib.ticker import AutoMinorLocator, LogLocator
from matplotlib.patches import Patch
from matplotlib.figure import Figure
from matplotlib.legend_handler import HandlerTuple
from typing import Optional, List, Iterable

def fig_ild_style(fig:Figure, xlim:Iterable, bins:int,
                  xscale:str='linear', xunit:Optional[str]='GeV', xlabel:str='m',
                  yscale:str='linear', yunit:Optional[str]='events', ylabel_prefix:str='',
                  fontname='Arial', beam_spec:bool=True, ax_index:int=0,
                  title:Optional[str]=None, title_postfix:str='',
                  legend_labels:Optional[List]=None, legend_kwargs={},
                  colorpalette:Optional[Iterable]=None,
                  ild_offset_x:float=0., ild_offset_y:float=0., ild_status:str='preliminary')->Figure:
    
    from phc import get_colorpalette
    if colorpalette is None:
        colorpalette = get_colorpalette()
    
    if yunit is None or yunit =='':
        yunit = '1'
        
    if title_postfix != '':
        title = title[:-1] + title_postfix + ')'
    
    fig.text(0.15+ild_offset_x, 0.83+ild_offset_y, f'ILD {ild_status}', fontsize=12, weight='bold', fontname=fontname)
    
    if beam_spec:
        fig.text(0.15+ild_offset_x, 0.795+ild_offset_y, r'$\sqrt{s} = 500$ GeV, $L_{int} = 2$ab$^{-1}$', fontsize=8, fontname=fontname)
        fig.text(0.15+ild_offset_x, 0.765+ild_offset_y, r'$P(e^{+}, e^{-}) = (+0.3, -0.8)$', fontsize=8, fontname=fontname)
    
    for ax in fig.get_axes():
        for label in ax.get_xticklabels():
            label.update({'fontname': fontname})
            
        for label in ax.get_yticklabels():
            label.update({'fontname': fontname})
            
    ax = fig.get_axes()[ax_index]
    
    if xscale =='linear':
        ax.xaxis.set_minor_locator(AutoMinorLocator(5))
    else:
        raise NotImplementedError('Log scale not yet implemented')
    
    if yscale =='log':
        ax.set_yscale('log')
        ax.yaxis.set_major_locator(LogLocator(base=10))
        ax.yaxis.set_minor_locator(LogLocator(base=10,subs=[2., 5., 6., 7., 8., 9.]))
    else:
        ax.yaxis.set_minor_locator(AutoMinorLocator(5))        
    
    ax.xaxis.set_ticks_position('both')
    ax.yaxis.set_ticks_position('both')
    
    ax.tick_params(axis='both', width=1.5, length=5, which='both', direction='in', labelsize=12)
    ax.tick_params(axis='both', width=2.5, length=8)
    
    ax.set_xlabel(rf'${xlabel}$' + ('' if (xunit == '' or xunit is None) else f' [{xunit}]'), fontsize=12, fontname=fontname)
    ax.set_ylabel(ylabel_prefix + rf"{yunit} / {(xlim[1]-xlim[0])/bins:0.2f}" + (f' {xunit}' if (xunit != '' and xunit is not None) else ''), fontsize=12, fontname=fontname)
    
    if title is not None:
        ax.set_title(title, loc='right', fontsize=12, fontname=fontname)
    
    if legend_labels is not None:
        cp = get_colorpalette()
        legend_handles = []
        for i in range(len(legend_labels)):
            process_name = legend_labels[i]
            legend_handles.append([Patch(facecolor=cp[i], edgecolor=cp[i], label=process_name)])
            
        ax.legend(handles=legend_handles, labels=legend_labels, fontsize=10, handler_map={list: HandlerTuple(ndivide=None, pad=0)}, **legend_kwargs)
        
    return fig