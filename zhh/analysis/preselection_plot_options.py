from matplotlib.ticker import MultipleLocator

preselection_plot_options = {
    'llbbbb': [
        { 'xlabel': 'IsoLeptons', 'plot_hist_kwargs': { 'int_bins': True }, 'ild_style_kwargs': { 'xminor': False, 'xlocator': MultipleLocator(1) } },
        { 'xlabel': 'm_{ll}', 'xlim': [40, 140], 'xunit': 'GeV' },
        { 'xlabel': 'm_{H1}', 'xlim': [0, 250], 'xunit': 'GeV' },
        { 'xlabel': 'm_{H2}', 'xlim': [0, 250], 'xunit': 'GeV' },
        { 'xlabel': 'p_{t}^{miss}', 'xlim': [0, 200], 'xunit': 'GeV' },
        { 'xlabel': 'thrust', 'xlim': [0, 1] },
        None
    ],
    'vvbbbb': [
        { 'xlabel': 'IsoLeptons', 'plot_hist_kwargs': { 'int_bins': True }, 'ild_style_kwargs': { 'xminor': False, 'xlocator': MultipleLocator(1) } },
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
        { 'xlabel': 'IsoLeptons', 'plot_hist_kwargs': { 'int_bins': True }, 'ild_style_kwargs': { 'xminor': False, 'xlocator': MultipleLocator(1) } },
        { 'xlabel': 'm_{H1}', 'xlim': [0, 250], 'xunit': 'GeV' },
        { 'xlabel': 'm_{H2}', 'xlim': [0, 250], 'xunit': 'GeV' },
        { 'xlabel': 'p_{t}^{miss}', 'xlim': [0, 200], 'xunit': 'GeV' },
        { 'xlabel': 'thrust' },
        { 'xlabel': 'bmax4' },
        None
    ]
}