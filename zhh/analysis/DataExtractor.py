import numpy as np
import matplotlib.pyplot as plt
from .CutflowProcessor import CutflowProcessor
from .AnalysisChannel import AnalysisChannel
from copy import deepcopy
from tqdm.auto import tqdm
from ..util.PlotContext import PlotContext

class DataExtractor:
    def __init__(self,
                 cutflow_processor:CutflowProcessor,
                 sources:list[AnalysisChannel]|None=None):
        
        self._cp = cutflow_processor
        self._sources = sources if sources is not None else cutflow_processor._sources
        self._features:list[str]|None = None
        self._labels:np.ndarray|None = None
        
    def extract(self,
                to_process:list[tuple[str, int]],
                features:list[str],
                MOD_WEIGHT:bool=True,
                step:int|None=None,
                split:int|None=None,
                weight_prop:str='weight')->tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
        """Extracts numpy arrays containing event weights, feature data and
        labels for the event categories contained in to_process.

        Args:
            to_process (list[tuple[str, int]]): list of tuples of structure
                [<categoryName>, <classLabel>]
            features (list[str]): list of features present in the data-
                stores of each AnalysisChannel (can be ROOT branches or
                features attached via store['feature'] = data)
            MOD_WEIGHT (bool, optional): _description_. Defaults to True.
            step (int | None, optional): _description_. Defaults to None.
            split (int | None, optional): which split to consider. Check
                apply_split for more info. If None, will not consider any
                splits. Defaults to None.
            weight_prop (str): which column to use for weights. Should be
                changed if the split option is used. For an example to cal-
                culate weights in this case, see mod_weights_from_split.

        Raises:
            Exception: _description_

        Returns:
            tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
                src_idx, event_num, labels, weight, inputs
        """
        
        if split is not None and weight_prop == 'weight':
            print('Warning: You seem to use the default weights and a split dataset at the same time. This may lead to wrong results. Consider creating a separate weight column for a set of splits. See the description of extract().')
        
        sources = self._sources
        self._features = features
        
        events_passed:dict[str, np.ndarray] = {}
        fs_2_source:dict[str, AnalysisChannel] = {}
        src_2_src_idx:dict[AnalysisChannel, int] = {}
        
        for src_idx, source in enumerate(sources):
            source.getStore().resetView()
            src_2_src_idx[source] = src_idx
            
        for fs_name, class_label in to_process:
            for source in sources:
                if source.containsFinalState(fs_name):
                    fs_2_source[fs_name] = source
                    break
            
            if not fs_name in fs_2_source:
                raise Exception(f'No source found for final state <{fs_name}>')

        nrows_tot = 0
        for fs_name, class_label in to_process:
            source = fs_2_source[fs_name]
            category_mask = source.getCategoryMask(fs_name)
            post_presel_mask = self._cp.getFinalEventMaskByName(source.getName(), step=step)
            
            mask = category_mask & post_presel_mask
            
            if split is not None:
                mask = mask & (source.getStore()['split'] == split)
            
            nrows_tot += int(mask.sum())
            
            events_passed[fs_name] = mask            

        src_idx = np.zeros(nrows_tot, dtype='B')
        event_num = np.zeros(nrows_tot, dtype='I')
        
        inputs = np.zeros((nrows_tot, len(features)))
        weight = np.zeros(nrows_tot)
        labels = np.zeros(nrows_tot, dtype='B')

        pointer = 0
        pbar = tqdm(range(nrows_tot))
        
        labels_unique = []
        
        for i, (fs_name, class_label) in enumerate(to_process):
            source = fs_2_source[fs_name]
            store = source.getStore()
            
            mask = events_passed[fs_name]
            nrows = int(mask.sum())
            
            src_idx[pointer:pointer + nrows] = src_2_src_idx[source]
            event_num[pointer:pointer + nrows] = store['event'][mask]
            
            pbar.set_description(f'Extracting features for <{source.getName()}.{fs_name}>')

            for i, feature in enumerate(features):
                data = store[feature][mask]                
                inputs[pointer:pointer + nrows, i] = data
            
            #weight[pointer:pointer + nrows] = (1/nrows * (1 if fs_name == 'μμbb' else 10)) if MOD_WEIGHT else store['weight'][mask]
            pbar.update(len(data))
            
            weight[pointer:pointer + nrows] = 1/nrows if MOD_WEIGHT else store[weight_prop][mask]
            labels[pointer:pointer + nrows] = class_label
            
            pointer += nrows
            labels_unique.append(class_label)
        
        pbar.close()
            
        self._labels = np.array(labels_unique, dtype='B')
            
        return src_idx, event_num, labels, weight, inputs
    
    def plot(self, inputs, weight, labels, filename:str|None='mva_inputs.pdf',
             label_2_category:dict[int, str]|None=None, context:PlotContext|None=None,
             signal_category:str|None=None, plot_options:dict[str, dict]={}):
        
        assert(self._features is not None and self._labels is not None)
        
        features = self._features
        
        from zhh import colormap_desy, plot_weighted_hist, figure_options, deepmerge
        from phc import export_figures
        
        if context is None:
            context = PlotContext(colormap_desy)
            
        if label_2_category is None:
            label_2_category = {}
            
            assert(0 in self._labels and 1 in self._labels)
            label_2_category[0] = 'Background'
            label_2_category[1] = 'Signal'
            
            signal_category = 'Signal'

        xunits:list[None|str] = [None] * len(features)
        #xunits[0] = 'GeV'
        
        hist_kwargs_overwrite = {}
        for label, category in label_2_category.items():
            if category == signal_category:
                hist_kwargs_overwrite[category] = {}
            else:
                hist_kwargs_overwrite[category] = { 'histtype': 'stepfilled', 'color': context.getColorByKey(category) }

        plot_kwargs_base = {
            'yscale': 'linear',
            'plot_hist_kwargs': {
                'stacked': False,
                'show_stats': False,
                'normalize': True,
                'hist_kwargs': { 'hatch': None }
            },
            'ild_style_kwargs': {
                'legend_kwargs': { 'loc': 'upper right', 'bbox_to_anchor': (.98, .98), 'fancybox': False }
            }
        }

        figures = []

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
            
            fig = plot_weighted_hist(plot_dict, title=f'MVA Input <{feature}>', xunit=xunits[i],
                                        plot_context=context, plot_hist_kwargs_overwrite=hist_kwargs_overwrite, **plot_kwargs)
            
            figures.append(fig)
            
            plt.close(fig)
            
        if filename is not None:
            export_figures(filename, figures)