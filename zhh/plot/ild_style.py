from matplotlib.ticker import AutoMinorLocator, LogLocator, Locator
from collections.abc import Collection
from typing import Literal, Union, TYPE_CHECKING
from ..util.get_matplotlib_fonts import resolve_fonts
from ..util.PlotContext import PlotContext
from ..util.deepmerge import deepmerge
from copy import deepcopy

if TYPE_CHECKING:
    from matplotlib.figure import Figure
    from matplotlib.axes import Axes

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

class ILDStyleContext:
    def __init__(self, beam_energy:int|float, lum_inv_ab:int|float, polarization:tuple[float, float]|None,
                 ild_status:str='preliminary', fontname:list[str]=['DejaVu Sans'],
                 colorpalette:list|None=None,
                 ild_offset_x:float|int=0., ild_offset_y:float|int=0.,
                 ild_offset_beamspec_mult:float=1):
        """Creates a context for applying ILD style with common descriptive values on
        matplotlib figures/axes.

        Args:
            beam_energy (int | float): center-of-mass energy in GeV, shown as sqrt(s)
            lum_inv_ab (int | float): integrated luminosity in ab^-1
            polarization (tuple[float, float] | None): polarization of (e-, e+). None
                or 0 for unpolarized beams.
            ild_status (str, optional): status shown next to ILD. Defaults to 'preliminary'.
            fontname (list[str], optional): fonts to try in order. Defaults to ['DejaVu Sans'].
            colorpalette (list | None, optional): if None, it will be fetched from a PlotContext during apply(), if supplied there. Defaults to None.
            ild_offset_x (float | int, optional): position of the ILD label in axes coordinates. Defaults to 0..
            ild_offset_y (float | int, optional): position of the ILD label in axes coordinates. Defaults to 0..
            ild_offset_beamspec_mult (float, optional): scales the spacing between the ILD label and the beam spec. Defaults to 1..
            
        """
        
        if polarization is None or polarization == 0:
            polarization = (0., 0.)
        
        self._beam_energy = beam_energy
        self._lum_inv_ab = lum_inv_ab
        self._polarization_em = polarization[0]
        self._polarization_ep = polarization[1]
        self._ild_status = ild_status
        self._fontname = fontname
        self._colorpalette = colorpalette
        self._ild_offset_x = ild_offset_x
        self._ild_offset_y = ild_offset_y
        self._ild_offset_beamspec_mult = ild_offset_beamspec_mult
    
    @classmethod
    def fromDefaults(cls, **kwargs)->'ILDStyleContext':
        """Creates a context with the values of ild_style_defaults (ZHH at 550 GeV),
        any of which may be overwritten by kwargs."""
        
        ctx_kwargs = {
            'beam_energy': ild_style_defaults['beam_energy'],
            'lum_inv_ab': ild_style_defaults['luminosity_inv_ab'],
            'polarization': (ild_style_defaults['polarization_em'], ild_style_defaults['polarization_ep']),
            'ild_status': ild_style_defaults['ild_status'],
            'fontname': ild_style_defaults['fontname'],
            'colorpalette': ild_style_defaults['colorpalette'],
            'ild_offset_x': ild_style_defaults['ild_offset_x'],
            'ild_offset_y': ild_style_defaults['ild_offset_y'],
        } | kwargs
        
        return cls(**ctx_kwargs)
    
    def getFont(self, context:PlotContext|None=None)->str:
        return context.getFont() if context is not None else resolve_fonts(self._fontname)

    def apply(self, fig_or_ax:Union['Figure', 'Axes'], xlim:list[float]|tuple[float,float]|None=None, bins:Collection|int|None=None,
                  xscale:str='linear', xunit:str|None='GeV', xlabel:str='m',
                  yscale:str='linear', yunit:str|None='events', ylabel_prefix:str='', y_label:str|None=None,
                  xlocator:Locator|None=None, ylocator:Locator|None=None,
                  xminor:int|None=5, yminor:int|None=5,

                  ax_index:int=0,
                  title:str|None=None, title_postfix:str='',
                  legend_labels:list|None=None,
                  legend_kwargs={},
                  colorpalette:list|None=None,
                  plot_context:PlotContext|None=None, int_bins:bool=False,
                  ild_text_position:Literal['upper left','upper right']|None='upper left',

                  beam_spec:bool=True,
                  ild_offset_x:float|None=None,
                  ild_offset_y:float|None=None,
                  ild_offset_beamspec_mult:float|None=None,
                  ild_status:str|None=None,
                  labelsize:int=12, titlesize:int|None=None,
                  show_binning_on_y_scale:bool|None=None,
                  columns:list[str]|None=None,
                  return_legend_kwargs:bool=False
                )->Union['Figure', tuple['Figure',dict]]:
        
        from matplotlib.figure import Figure
        from matplotlib.axes import Axes
        
        ild_offset_x = self._ild_offset_x if ild_offset_x is None else ild_offset_x
        ild_offset_y = self._ild_offset_y if ild_offset_y is None else ild_offset_y
        ild_offset_beamspec_mult = self._ild_offset_beamspec_mult if ild_offset_beamspec_mult is None else ild_offset_beamspec_mult
        ild_status = self._ild_status if ild_status is None else ild_status
        colorpalette = self._colorpalette if colorpalette is None else colorpalette
        
        if isinstance(fig_or_ax, Axes):
            fig = fig_or_ax.figure
            ax = fig_or_ax
        elif isinstance(fig_or_ax, Figure):
            fig = fig_or_ax
            ax = fig.get_axes()[ax_index]
        else:
            raise TypeError('fig_or_ax must be a Figure or Axes instance')
        
        assert(isinstance(fig, Figure))
        
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
        
        fontname = self.getFont(plot_context)
        
        ax.text(ild_offset_x, ild_offset_y, f'ILD {ild_status}', fontsize=12, weight='bold', fontname=fontname, transform=ax.transAxes)
        
        if beam_spec:
            ax.text(ild_offset_x, ild_offset_y-.035*ild_offset_beamspec_mult, rf'$\sqrt{{s}} = {self._beam_energy}$ GeV, $L_{{int}} = {self._lum_inv_ab}$ab$^{{-1}}$', fontsize=8, fontname=fontname, transform=ax.transAxes)
            ax.text(ild_offset_x, ild_offset_y-.065*ild_offset_beamspec_mult, rf'$P(e^{{+}}, e^{{-}}) = ({self._polarization_ep:+.1f}, {self._polarization_em:+.1f})$', fontsize=8, fontname=fontname, transform=ax.transAxes)
            
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
        
        if y_label is None:
            y_label = ylabel_prefix + (rf'{yunit}' if (yunit is not None and yunit != '') else '')
            if is_hist and show_binning_on_y_scale:
                assert(xlim is not None and bins is not None)
                
                nbins = (len(bins) if isinstance(bins, Collection) else bins) if not int_bins else int(round(xlim[1] - xlim[0]))
                ylabel_binning_val = (xlim[1]-xlim[0])/nbins
                ylabel_binning_str = (rf'{ylabel_binning_val:.2f}' if ylabel_binning_val >= 0.1 else rf'{ylabel_binning_val:.2E}') if not int_bins else f'bin'
                y_label += rf' / {ylabel_binning_str}'
                
                if (xunit != '' and xunit is not None):
                    y_label += f' {xunit}'
                
        x_label = rf'${xlabel}$ [{xunit}]' if (xunit is not None and xunit != '') else xlabel
        
        legend_call_kwargs = {}
        if is_hist and legend_labels is not None:
            assert(isinstance(colorpalette, list))

            legend_call_kwargs = deepmerge(self.legend_kwargs_fn(plot_context), deepcopy(legend_kwargs))
            
            ax.legend(**legend_call_kwargs)
        
        self.update_plot(ax, x_label=x_label, y_label=y_label, title=title, labelsize=labelsize, titlesize=titlesize, context=plot_context)
        
        if return_legend_kwargs:
            return fig, legend_call_kwargs
        else:
            return fig

    def update_plot(self, ax:'Axes', x_label:str|None=None, y_label:str|None=None,
                    title:str|None=None, fontname:str|None=None, context:PlotContext|None=None,
                    labelsize:int=12, titlesize:int|None=None,
                    x_tick_label_size:int|None=None, y_tick_label_size:int|None=None, tick_label_size:int|None=None):
        """Applies the font (fontname > context > fonts of this ILDStyleContext) and
        sizes to tick labels, axis labels and title. Labels/title which are None are
        taken from ax, if already set."""
        
        if x_tick_label_size is None and tick_label_size is not None:
            x_tick_label_size = tick_label_size
        
        if y_tick_label_size is None and tick_label_size is not None:
            y_tick_label_size = tick_label_size
        
        if titlesize is None:
            titlesize = labelsize
        
        if fontname is None:
            fontname = self.getFont(context)
            
        assert(fontname is not None)
        
        for label in ax.get_xticklabels():
            label.set_fontname(fontname)
            
            if x_tick_label_size is not None:
                label.set_fontsize(x_tick_label_size)
            
        for label in ax.get_yticklabels():
            label.set_fontname(fontname)
            
            if y_tick_label_size is not None:
                label.set_fontsize(y_tick_label_size)
            
        x_label = x_label if x_label is not None else ax.get_xlabel()
        x_label = x_label if x_label != '' else None
            
        if x_label is not None:
            ax.set_xlabel(x_label, fontsize=labelsize, fontname=fontname)
            
        y_label = y_label if y_label is not None else ax.get_ylabel()
        y_label = y_label if y_label != '' else None
            
        if y_label is not None:
            ax.set_ylabel(y_label, fontsize=labelsize, fontname=fontname)
        
        title = title if title is not None else ax.get_title()
        title = title if title != '' else None
        
        if title is not None:
            ax.set_title(title, loc='right', fontsize=titlesize, fontname=fontname)

    def legend_kwargs_fn(self, context:PlotContext|None=None, size:int=9, titlesize:int=10)->dict:
        """Returns kwargs for ax.legend() using the font of context or, if None,
        the fonts of this ILDStyleContext."""
        
        fontname = self.getFont(context)
        
        return {
            'columnspacing': 0,
            'fancybox': False,
            'prop': {
                'family': fontname,
                'size': size,
            },
            'title_fontproperties': {'family': fontname, 'size': titlesize },
        }

def fig_ild_style(fig_or_ax:Union['Figure', 'Axes'], *args,
                  beam_energy:int|float=ild_style_defaults['beam_energy'],
                  colorpalette:list|None=ild_style_defaults['colorpalette'],
                  **kwargs)->Union['Figure', tuple['Figure',dict]]:
    """Deprecated shorthand for ILD Style with ZHH 550 GeV values (see ild_style_defaults).
    Where used, it should be replaced with ILDStyleContext.apply().
    
    args and kwargs are passed on to ILDStyleContext.apply().
    """
    
    return ILDStyleContext.fromDefaults(beam_energy=beam_energy).apply(fig_or_ax, *args, colorpalette=colorpalette, **kwargs)

def update_plot(ax:'Axes', x_label:str|None=None, y_label:str|None=None, title:str|None=None, fontname:str|None=None, context:PlotContext|None=None,
                labelsize:int=12, titlesize:int|None=None, x_tick_label_size:int|None=None, y_tick_label_size:int|None=None, tick_label_size:int|None=None):
    """Legacy shorthand for ILDStyleContext.update_plot() with default values."""
    
    return ILDStyleContext.fromDefaults().update_plot(ax, x_label=x_label, y_label=y_label, title=title, fontname=fontname, context=context,
                                                      labelsize=labelsize, titlesize=titlesize, x_tick_label_size=x_tick_label_size,
                                                      y_tick_label_size=y_tick_label_size, tick_label_size=tick_label_size)

def legend_kwargs_fn(context:PlotContext|None=None, size:int=9, titlesize:int=10):
    """Legacy shorthand for ILDStyleContext.legend_kwargs_fn() with default values."""
    
    return ILDStyleContext.fromDefaults().legend_kwargs_fn(context, size=size, titlesize=titlesize)

def fancify_formula(s:str):
    for (a, b) in [
        ('>=', ' \\geq '),
        ('<=', ' \\leq '),
    ]:
        s = s.replace(a, b)
    
    return s