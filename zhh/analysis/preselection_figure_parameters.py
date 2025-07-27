from matplotlib.ticker import MultipleLocator

preselection_plot_options:dict[str, list[dict]]= {
    'eebbbb': [
        { 'xlabel': 'ISOElectrons',
            'plot_hist_kwargs': { 'int_bins': True },
            'ild_style_kwargs': { 'xminor': False, 'xlocator': MultipleLocator(1), 'ild_text_position': 'upper right' } },
        { 'xlabel': 'pairedLepType',
            'plot_hist_kwargs': { 'int_bins': True },
            'ild_style_kwargs': { 'xminor': False, 'xlocator': MultipleLocator(1), 'legend_kwargs': { 'loc': 'center' } } },
        { 'xlabel': 'm_{ll}', 'xlim': [0, 300], 'xunit': 'GeV' },
        { 'xlabel': 'm_{H1}', 'xlim': [0, 300], 'xunit': 'GeV' },
        { 'xlabel': 'm_{H2}', 'xlim': [0, 300], 'xunit': 'GeV' },
        { 'xlabel': 'p_{t}^{miss}', 'xlim': [0, 300], 'xunit': 'GeV' },
        { 'xlabel': 'thrust', 'xlim': [0, 1] },
        { 'xlabel': 'sum b-tags', 'xlim': [0, 4] }
    ],
    'µµbbbb': [
        { 'xlabel': 'ISOMuons',
            'plot_hist_kwargs': { 'int_bins': True },
            'ild_style_kwargs': { 'xminor': False, 'xlocator': MultipleLocator(1), 'ild_text_position': 'upper right' } },
        { 'xlabel': 'pairedLepType',
            'plot_hist_kwargs': { 'int_bins': True },
            'ild_style_kwargs': { 'xminor': False, 'xlocator': MultipleLocator(1), 'legend_kwargs': { 'loc': 'center' } } },
        { 'xlabel': 'm_{ll}', 'xlim': [0, 300], 'xunit': 'GeV' },
        { 'xlabel': 'm_{H1}', 'xlim': [0, 300], 'xunit': 'GeV' },
        { 'xlabel': 'm_{H2}', 'xlim': [0, 300], 'xunit': 'GeV' },
        { 'xlabel': 'p_{t}^{miss}', 'xlim': [0, 300], 'xunit': 'GeV' },
        { 'xlabel': 'thrust', 'xlim': [0, 1] },
        { 'xlabel': 'sum b-tags', 'xlim': [0, 4] }
    ],
    'vvbbbb': [
        { 'xlabel': 'IsoLeptons', 'plot_hist_kwargs': { 'int_bins': True }, 'ild_style_kwargs': { 'xminor': False, 'xlocator': MultipleLocator(1) } },
        { 'xlabel': 'm_{H1}', 'xlim': [0, 250], 'xunit': 'GeV' },
        { 'xlabel': 'm_{H2}', 'xlim': [0, 250], 'xunit': 'GeV' },
        { 'xlabel': 'p_{t}^{miss}', 'xlim': [0, 200], 'xunit': 'GeV' },
        { 'xlabel': 'thrust' },
        { 'xlabel': 'E_{vis}' },
        { 'xlabel': 'm_{HH}', 'xunit': 'GeV' },
        { 'xlabel': 'bmax3' }
    ],
    'qqbbbb': [
        { 'xlabel': 'IsoLeptons', 'plot_hist_kwargs': { 'int_bins': True }, 'ild_style_kwargs': { 'xminor': False, 'xlocator': MultipleLocator(1) } },
        { 'xlabel': 'm_{H1}', 'xlim': [0, 250], 'xunit': 'GeV' },
        { 'xlabel': 'm_{H2}', 'xlim': [0, 250], 'xunit': 'GeV' },
        { 'xlabel': 'p_{t}^{miss}', 'xlim': [0, 200], 'xunit': 'GeV' },
        { 'xlabel': 'thrust' },
        { 'xlabel': 'bmax4' }
    ]
}

preselection_table_options = {
    
}
