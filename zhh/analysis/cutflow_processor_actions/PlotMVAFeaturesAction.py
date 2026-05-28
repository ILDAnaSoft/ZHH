from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
from ...util.PlotContext import PlotContext
from typing import Literal
import numpy as np
import os, os.path as osp

class PlotMVAFeaturesAction(FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, mva:str, file:str,
                 plot_which:Literal['train','test']='train', plot_kwargs:dict={}, **kwargs):
        
        """Creates plots of all the input distributions for MVA training. Uses the data written out by WriteMVADataAction.

        Required arguments:

        - (str) `mva`: name of the MVA registered in cutflow
        - (str) `file`: name of the output PDF file

        Optional arguments:
        - (Literal['train', 'test']) `plot_which`: which dataset should be plotted. Defaults to 'train'.
        - (dict) `plot_kwargs`: base options to control the plotting by plot_weighted_hist(). other plotting optins are merged into a copy of this. Defaults to an empty dict.

        Raises:
            Exception: _description_
        """
        assert('mvas' in steer)

        super().__init__(cp, steer)

        from zhh import EventCategories, find_by

        mva_spec = find_by(steer['mvas'], 'name', mva, is_dict=True)

        self._data_file = mva_spec['data_file']
        self._plot_file = file
        self._kwargs = kwargs

        self._signal_categories = [EventCategories.inverted[x] for x in steer['signal_categories']]

        label_2_category = {}
        for label, category in mva_spec['classes']:
            label_2_category[label] = category

        self._label_2_category = label_2_category
        self._plot_kwargs = plot_kwargs

        if not plot_which in ['train', 'test']:
            raise Exception('plot_which must be either `train` or `test`')

        self._plot_which = plot_which
    
    def run(self):
        from phc import export_figures
        import matplotlib.pyplot as plt

        os.makedirs(self.output().absdirname, exist_ok=True)

        data = np.load(self._data_file)

        features = data['features']
        inputs = data[f'X_{self._plot_which}']
        weight = data[f'w_{self._plot_which}']
        labels = data[f'y_{self._plot_which}']

        figures = plotFn(
            features, inputs, labels, weight,
            self._label_2_category, self._signal_categories,
            plot_kwargs=self._plot_kwargs
        )

        export_figures(self.output().abspath, figures)

        for fig in figures:
            plt.close(fig)
    
    def output(self):
        return self.localTarget(self._plot_file)

def plotFn(features, inputs, labels, weight, label_2_category:dict[int, str], signal_categories:list[str],
           plot_context:PlotContext|None=None, plot_options:dict[str, dict]={}, plot_kwargs:dict={}):
    
    from zhh import colormap_desy, plot_weighted_hist, figure_options, deepmerge, PlotContext
    from copy import deepcopy
    from tqdm.auto import tqdm
    from matplotlib.figure import Figure

    if plot_context is None:
        plot_context = PlotContext(colormap_desy)

    xunits:list[None|str] = [None] * len(features)
    #xunits[0] = 'GeV'

    hist_kwargs_overwrite = {}
    for label, category in label_2_category.items():
        is_signal = signal_categories is not None and category in signal_categories
        
        if is_signal:
            hist_kwargs_overwrite[category] = { 'histtype': 'step' }
            hist_kwargs_overwrite[category]['edgecolor'] = plot_context.getColorByKey(category)
            
        else:
            hist_kwargs_overwrite[category] = { 'histtype': 'stepfilled' }
            hist_kwargs_overwrite[category]['facecolor'] = plot_context.getColorByKey(category)
            #hist_kwargs_overwrite[category]['facecolor'] = plot_context.getColorByKey(category)
            hist_kwargs_overwrite[category]['alpha'] = 0.5

    plot_kwargs_base = deepmerge({
        'yscale': 'linear',
        'plot_hist_kwargs': {
            'stacked': False,
            'show_stats': False,
            'normalize': True,
            'hist_kwargs': { 'hatch': None },
            'figsize': (6, 5)
        },
        'ild_style_kwargs': {
            'legend_kwargs': { 'loc': 'upper right', 'bbox_to_anchor': (.98, .98), 'fancybox': False },
            'labelsize': 15,
            'ild_offset_beamspec_mult': 1.05,
            'ylabel_prefix': 'fraction in dataset',
            'yunit': ''
        }
    }, deepcopy(plot_kwargs))

    figures:list[Figure] = []

    for i, feature in enumerate((pbar := tqdm(features))):
        pbar.set_description(f'Plotting {feature}')
        
        plot_dict = {}
        
        for label, category in label_2_category.items():
            plot_dict[category] = ( inputs[labels == label][:, i], weight[labels == label] )
            
        plot_kwargs = deepcopy(plot_kwargs_base)
        plot_kwargs['xlabel'] = feature
        
        if feature in figure_options['default']:
            plot_kwargs = deepmerge(plot_kwargs, figure_options['default'][feature])
            
        if feature in plot_options:
            plot_kwargs = deepmerge(plot_kwargs, plot_options[feature])
        
        if 'xunit' not in plot_kwargs:
            plot_kwargs['xunit'] = xunits[i]
        
        fig = plot_weighted_hist(plot_dict, title=None, plot_context=plot_context,
                                    plot_hist_kwargs_overwrite=hist_kwargs_overwrite, **plot_kwargs)
        fig.set_layout_engine('tight')
        
        figures.append(fig)
    
    return figures