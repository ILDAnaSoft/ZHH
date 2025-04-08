from common import *

from matplotlib.ticker import MultipleLocator
from phc import plot_hist, export_figures, set_colorpalette
from zhh import fig_ild_style, plot_preselection_by_event_categories, \
    calc_preselection_by_event_categories, plot_preselection_by_calc_dict, PlotContext, \
    Cut, WindowCut, LessThanEqualCut, GreaterThanEqualCut, EqualCut, zhh_cuts, \
    colorpalette_desy, annotate_cut, weighted_counts_by_categories, \
    plot_total_efficiency, plot_cut_efficiencies, annotate_cut, get_matplotlib_fonts

from zhh.plot.ild_style import ild_style_defaults
ild_style_defaults['fontname'] = ['Montserrat', 'Liberation Sans', 'Droid Sans']

def apply_event_categories(data:np.ndarray):
    data['event_category'][data['process'] == ProcessCategories.e1e1hh] = EventCategories.eeHH
    data['event_category'][data['process'] == ProcessCategories.e2e2hh] = EventCategories.µµHH
    data['event_category'][data['process'] == ProcessCategories.e3e3hh] = EventCategories.𝜏𝜏HH

    data['event_category'][(data['process'] == ProcessCategories.e1e1hh) & (data['Nb_from_H'] == 4)] = EventCategories.eeHHbbbb
    data['event_category'][(data['process'] == ProcessCategories.e2e2hh) & (data['Nb_from_H'] == 4)] = EventCategories.µµHHbbbb
    data['event_category'][(data['process'] == ProcessCategories.e3e3hh) & (data['Nb_from_H'] == 4)] = EventCategories.𝜏𝜏HHbbbb

    data['event_category'][data['process'] == ProcessCategories.n1n1hh] = EventCategories.vvHH
    data['event_category'][data['process'] == ProcessCategories.n23n23hh] = EventCategories.vvHH

    data['event_category'][data['process'] == ProcessCategories.n1n1qqh] = EventCategories.vvqqH
    data['event_category'][data['process'] == ProcessCategories.n23n23qqh] = EventCategories.vvqqH

    # Fix backgrounds
    data['event_category'][data['process'] == ProcessCategories.e1e1qqh] = EventCategories.llqqH
    data['event_category'][data['process'] == ProcessCategories.e2e2qqh] = EventCategories.llqqH
    data['event_category'][data['process'] == ProcessCategories.e3e3qqh] = EventCategories.llqqH

    data['event_category'][data['process'] == ProcessCategories.qqqqh] = EventCategories.qqqqH

plot_options = {
    'llbbbb': {
        'nisoleps': { 'xlabel': 'IsoLeptons', 'plot_hist_kwargs': { 'int_bins': True },
                        'ild_style_kwargs': { 'xminor': False, 'xlocator': MultipleLocator(1), 'show_binning_on_y_scale': False, 'ild_text_position': 'upper right' } },
        'mz'      : { 'xlabel': 'm_{ll}', 'xlim': [40, 140], 'xunit': 'GeV' },
        'mh1'     : { 'xlabel': 'm_{H1}', 'xlim': [0, 250], 'xunit': 'GeV' },
        'mh2'     : { 'xlabel': 'm_{H2}', 'xlim': [0, 250], 'xunit': 'GeV' },
        'pt_miss' : { 'xlabel': 'p_{t}^{miss}', 'xlim': [0, 200], 'xunit': 'GeV' },
        'thrust'  : { 'xlabel': 'thrust' }
    },
    'vvbbbb': [
        { 'xlabel': 'IsoLeptons', 'plot_hist_kwargs': { 'int_bins': True }, 'ild_style_kwargs': { 'xminor': False, 'xlocator': MultipleLocator(1), 'show_binning_on_y_scale': False } },
        { 'xlabel': 'm_{H1}', 'xlim': [0, 250], 'xunit': 'GeV' },
        { 'xlabel': 'm_{H2}', 'xlim': [0, 250], 'xunit': 'GeV' },
        { 'xlabel': 'p_{t}^{miss}', 'xlim': [0, 200], 'xunit': 'GeV' },
        { 'xlabel': 'thrust' },
        { 'xlabel': 'E_{vis}' },
        { 'xlabel': 'm_{HH}', 'xunit': 'GeV' },
        { 'xlabel': 'bmax3' },
        None
    ],
    'qqbbbb': [
        { 'xlabel': 'IsoLeptons', 'plot_hist_kwargs': { 'int_bins': True }, 'ild_style_kwargs': { 'xminor': False, 'xlocator': MultipleLocator(1), 'show_binning_on_y_scale': False } },
        { 'xlabel': 'm_{H1}', 'xlim': [0, 250], 'xunit': 'GeV' },
        { 'xlabel': 'm_{H2}', 'xlim': [0, 250], 'xunit': 'GeV' },
        { 'xlabel': 'p_{t}^{miss}', 'xlim': [0, 200], 'xunit': 'GeV' },
        { 'xlabel': 'thrust' },
        { 'xlabel': 'bmax4' },
        None
    ]
}