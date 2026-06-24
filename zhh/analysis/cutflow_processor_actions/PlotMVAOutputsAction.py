from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
from ...util.PlotContext import PlotContext
from typing import Literal
import numpy as np
import os, os.path as osp

class PlotMVAOutputsAction(FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, mva:str, file:str, split:int|None=None, step:int|None=None, signal_categories:list[str]|None=None,
                 weight_column:str|None=None, nbins:int=25, N_largest:int=3, **kwargs):
        
        assert('mvas' in steer)

        super().__init__(cp, steer)

        from zhh import EventCategories, find_by

        mva_spec = find_by(steer['mvas'], 'name', mva, is_dict=True)

        self._plot_file = file

        self._mva_label = label_name = mva_spec['label_name']
        self._mva_classes = mva_categories = [ item[1] for item in mva_spec['classes'] ]
        self._columns = [f'{label_name}#{class_name}' for class_name in mva_categories] + ['fsc_coded']

        self._signal_categories = signal_categories = signal_categories if signal_categories is not None else steer['signal_categories']

        if not len(signal_categories):
            raise Exception('At least one signal category must be defined')

        found_signal_categories = list(set(self._mva_classes) & set(self._signal_categories))
        if len(found_signal_categories) != len(self._signal_categories):
            print(f'Warning: Did not find categories {list(set(self._signal_categories) - set(found_signal_categories))} in MVA classes.' + \
                  'Optimization performance may be severed because the MVA did not take the signal category into account.')
            
        self._signal_category_ids = [ self._mva_classes.index(category) for category in found_signal_categories ]

        self._split = split
        self._step = step
        self._weight_column = weight_column
        self._nbins = nbins
        self._N_largest = N_largest
    
    def run(self):
        from phc import export_figures
        from zhh import DataSource
        import matplotlib.pyplot as plt

        # lazily fetching weight_column
        weight_column = self._weight_column
        step = self._step

        if weight_column is None:
            if step is None or not step in self._cp._weight_columns:
                raise Exception(f'weight_column is a required argument if step is not given or no weight_column for step={step} was registered in the CutflowProcessor before (did you forget to run ApplyCuts beforehand?)')
            
            weight_column = self._cp._weight_columns[step]

        os.makedirs(self.output().absdirname, exist_ok=True)

        step = self._step if self._step is not None else list(self._cp._masks.keys())[-1]

        masks, data, weights, _1, _2 = self._cp.snapshotAt(self._columns, step, weight_column, self._split)
        is_signal, is_background = DataSource.signal_background_masks(self._cp._sources, masks, self._signal_categories)

        figures = plotFn(data, data['fsc_coded'], is_signal, is_background, weights, self._signal_category_ids, self._columns[:-1],
                         nbins=self._nbins, N_largest=self._N_largest)

        export_figures(self.output().abspath, figures)

        for fig in figures:
            plt.close(fig)
    
    def output(self):
        return self.localTarget(self._plot_file)

def plotFn(probas:dict[str, np.ndarray], fsc_coded:np.ndarray, is_signal:np.ndarray, is_background:np.ndarray, weights:np.ndarray, categories:list[int], mva_score_names:list[str],
           nbins:int=25, N_largest:int=3, plot_context:PlotContext|None=None, plot_additional:list[tuple[bool, float, float, bool]]=[]):
    
    from zhh import PlotContext, plot_combined_hist, FinalStateCounts
    from matplotlib.figure import Figure
    import matplotlib.pyplot as plt

    if plot_context is None:
        plot_context = PlotContext()

    figures:list[Figure] = []
    
    plot_options = [
        (False, 0, 1, False),
        (False, 0, 1, True),
        (False, 0, 0.1, False),
        (True, -5, 0, False),
        (True, -5, 0, True),
    ] + plot_additional

    for logscale, bin_min, bin_max, by_fs in plot_options:
        bins = np.logspace(bin_min, bin_max, nbins) if logscale else np.linspace(bin_min, bin_max, nbins)
        zoomed_in = bins[0] > 1e-9 or abs(bins[-1] - 1) > 1e-9
        op = 'sum' if zoomed_in else 'tot'

        fig, axes = plt.subplots(figsize=(9, len(categories) * 5), nrows=len(categories), ncols=2)
        fig.set_layout_engine('constrained')


        for j, category in enumerate(categories):
            category_name = mva_score_names[category]

            # collect count by final state
            if by_fs:
                final_states_signal:dict[str, np.ndarray] = {}
                final_states_background:dict[str, np.ndarray] = {}

                for label, mask in [
                    ('Signal', is_signal),
                    ('Background', is_background)
                ]:
                    proba = probas[category_name]
                    selection = mask if not zoomed_in else (mask & (proba >= bins[0]) & (proba <= bins[-1]))
                    final_states = FinalStateCounts.encoded_by_frequency(fsc_coded[selection], weights[selection])
                    covered = np.zeros(len(mask), dtype=bool)

                    for i in range(N_largest + 1):
                        if i != N_largest - 1:
                            fs_id, fs_count = final_states[i]
                            fs_label = FinalStateCounts.decode_fancy_formula(fs_id)
                            fs_mask = fsc_coded == fs_id
                        else:
                            fs_mask = ~covered
                            fs_label = 'other'

                        (final_states_signal if label == 'Signal' else final_states_background)[fs_label] = fs_mask
                        covered = np.logical_or(covered, fs_mask)

            max_value = 0
            for mask in [is_signal, is_background]:
                hist, edges = np.histogram(probas[category_name][mask], weights=weights[mask], bins=bins)
                max_value = max(max_value, np.max(hist))

            for i, (label, mask) in enumerate([
                ('Signal', is_signal),
                ('Background', is_background)
            ]):
                ax = axes[j, i]

                plot_dict = {}
                ild_style_kwargs = {
                    'ild_offset_x': 0.07,
                    'ild_offset_y': 0.92,
                    'y_label': None if label == 'Signal' else '',
                    #'legend_kwargs': { 'framealpha': 0.7 }
                    'legend_kwargs': { 'loc': 'upper left', 'bbox_to_anchor': (0., -0.13 if by_fs else -0.12) }
                }
                plot_hist_kwargs = {
                    'stacked': False,
                    'xscale': 'log' if logscale else 'linear',
                    'show_stats': False,
                    'ylim': [5e-3, 1.05*max_value],
                    'labels': []
                }

                if by_fs:
                    proba = probas[category_name]
                    selection = mask if not zoomed_in else (mask & (proba >= bins[0]) & (proba <= bins[-1]))

                    for fs_label, fs_mask in (final_states_signal if label == 'Signal' else final_states_background).items():
                        sum_total = weights[mask & fs_mask].sum()
                        sum_visible = weights[selection & fs_mask].sum()
                        
                        plot_dict[fs_label] = ( proba[selection & fs_mask], weights[selection & fs_mask] )
                        plot_hist_kwargs['labels'] += [f'{fs_label} ({op}: {sum_visible:.3g}' + ( ')' if not zoomed_in else f', {sum_visible/sum_total:.0%})')]

                    ild_style_kwargs['title'] = label

                else:
                    plot_dict[label] = ( probas[category_name][mask], weights[mask] )
                    sum_total = weights[mask].sum()
                    sum_visible = weights[mask][(
                        probas[category_name][mask] >= bins[0]) & (
                        probas[category_name][mask] <= bins[-1]
                    )].sum()
                    plot_hist_kwargs['labels'] += [f'{label} ({op}: {sum_visible:.3g}' + ( ')' if sum_total == sum_visible else f', {sum_visible/sum_total:.0%})')]

                plot_combined_hist(plot_dict, f'{category_name} score', plot_context, bins=bins, yscale='log',
                                    ax=ax, xlim=(bins[0], bins[-1]), plot_hist_kwargs=plot_hist_kwargs, ild_style_kwargs=ild_style_kwargs)
                
                ax.set_ylim([5e-3, 1.1 * max_value])
                ax.grid(which='major', linewidth=0.5)

        figures += [fig]

    return figures