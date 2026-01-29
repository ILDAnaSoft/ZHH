import numpy as np
import matplotlib.pyplot as plt
from .CutflowProcessor import CutflowProcessor
from .DataSource import DataSource
from copy import deepcopy
from tqdm.auto import tqdm
from ..util.PlotContext import PlotContext

class DataExtractor:
    def __init__(self,
                 cutflow_processor:CutflowProcessor,
                 sources:list[DataSource]|None=None):
        
        self._cp = cutflow_processor
        self._sources = sources if sources is not None else cutflow_processor._sources
        self._features:list[str]|None = None
        self._labels:np.ndarray|None = None
        
    def extract(self,
                to_process:list[tuple[int, str]],
                features:list[str],
                step:int|None=None,
                split:int|None=None,
                weight_prop:str='weight',
                shuffle:bool=True,
                dtype=np.float32)->tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
        """Extracts numpy arrays containing event weights, feature data and
        labels for the event categories contained in to_process.

        Args:
            to_process (list[tuple[str, int]]): list of tuples of structure
                [<categoryName>, <classLabel>]
            features (list[str]): list of features present in the data-
                stores of each DataSource (can be ROOT branches or
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
            tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
                src_idx, event_num, labels, weight, weight_phys, inputs

            src_idx, event_num, labels, weight, inputs
            weight gives the weight for training and weight_phys the "physical" weight used
            for the event counting
        """
        
        if split is not None and weight_prop == 'weight':
            print('Warning: You seem to use the default weights and a split dataset at the same time. This may lead to wrong results. Consider creating a separate weight column for a set of splits. See the description of extract().')
        
        sources = self._sources
        self._features = features
        
        events_passed:dict[str, np.ndarray] = {}
        fs_2_source:dict[str, DataSource] = {}
        src_2_src_idx:dict[DataSource, int] = {}
        
        for src_idx, source in enumerate(sources):
            source.getStore().resetView()
            src_2_src_idx[source] = src_idx
            
        for class_label, fs_name in to_process:
            for source in sources:
                if source.containsFinalState(fs_name):
                    fs_2_source[fs_name] = source
                    break
            
            if not fs_name in fs_2_source:
                raise Exception(f'No source found for final state <{fs_name}>')

        nrows_tot = 0
        for class_label, fs_name in to_process:
            source = fs_2_source[fs_name]
            category_mask = source.getCategoryMask(fs_name)
            post_cut_mask = self._cp.getFinalEventMaskByName(source.getName(), step=step)
            
            mask = category_mask & post_cut_mask
            
            if split is not None:
                mask = mask & (source.getStore()['split'] == split)
            
            if mask.sum() == 0:
                print(f'Warning: No events considered for class <{fs_name}> with label <{class_label}> in split <{split}>')

            nrows_tot += int(mask.sum())
            
            events_passed[fs_name] = mask

        src_idx = np.zeros(nrows_tot, dtype='B')
        event_num = np.zeros(nrows_tot, dtype='I')
        
        inputs = np.zeros((nrows_tot, len(features)), dtype=dtype)
        weight = np.zeros(nrows_tot, dtype=dtype)
        weight_phys = np.zeros(nrows_tot, dtype=dtype)
        labels = np.zeros(nrows_tot, dtype='B')

        pointer = 0
        pbar = tqdm(range(nrows_tot * len(features)))
        #print('nrows_tot=', nrows_tot)
        
        labels_unique = []
        
        for i, (class_label, fs_name) in enumerate(to_process):
            source = fs_2_source[fs_name]
            store = source.getStore()
            
            mask = events_passed[fs_name]
            nrows = int(mask.sum())
            #print('nrows=', nrows)

            if nrows:
                src_idx[pointer:pointer + nrows] = src_2_src_idx[source]
                event_num[pointer:pointer + nrows] = store['event'][mask]

                for i, feature in enumerate(features):
                    pbar.set_description(f'Extracting data for <{source.getName()}.{fs_name}> feature={feature}')
                    pbar.update(nrows)

                    inputs[pointer:pointer + nrows, i] = store[feature][mask]
                
                # see https://scikit-learn.org/stable/modules/generated/sklearn.utils.class_weight.compute_sample_weight.html
                weight     [pointer:pointer + nrows] = (1/nrows)*len(labels)/len(to_process)
                weight_phys[pointer:pointer + nrows] = store[weight_prop][mask]
                labels     [pointer:pointer + nrows] = class_label
            
                pointer += nrows

            labels_unique.append(class_label)
        
        pbar.close()
            
        self._labels = np.array(labels_unique, dtype='B')
        
        if shuffle:
            shuffled_indices = np.arange(len(labels))
    
            rng = np.random.default_rng(42)
            rng.shuffle(shuffled_indices)

            src_idx = src_idx[shuffled_indices]
            event_num = event_num[shuffled_indices]
            labels = labels[shuffled_indices]
            weight = weight[shuffled_indices]
            weight_phys = weight_phys[shuffled_indices]
            inputs = inputs[shuffled_indices]
            
        return src_idx, event_num, labels, weight, weight_phys, inputs
    
    def plot(self, inputs, weight, labels, filename:str|None='mva_inputs.pdf',
             label_2_category:dict[int, str]|None=None, context:PlotContext|None=None,
             signal_categories:list|None=None, plot_options:dict[str, dict]={},
             bkg_hist_kwargs:dict={ 'histtype': 'stepfilled' }):
        
        return plotFn(self, inputs, weight, labels, filename=filename,
                      label_2_category=label_2_category, context=context,
                      signal_categories=signal_categories, plot_options=plot_options,
                      bkg_hist_kwargs=bkg_hist_kwargs)
            
def plotFn(de:DataExtractor, inputs:np.ndarray, weight:np.ndarray, labels:np.ndarray,
           filename:str|None='mva_inputs.pdf', label_2_category:dict[int, str]|None=None,
           context:PlotContext|None=None, signal_categories:list|None=None,
           plot_options:dict[str, dict]={},
           bkg_hist_kwargs:dict={ 'histtype': 'stepfilled' }):
        
        assert(de._features is not None and de._labels is not None)
        
        features = de._features
        
        from zhh import colormap_desy, plot_weighted_hist, figure_options, deepmerge
        from phc import export_figures
        
        if context is None:
            context = PlotContext(colormap_desy)
            
        if label_2_category is None:
            label_2_category = {}
            
            assert(0 in de._labels and 1 in de._labels)
            label_2_category[0] = 'Background'
            label_2_category[1] = 'Signal'
            
            signal_categories = ['Signal']

        xunits:list[None|str] = [None] * len(features)
        #xunits[0] = 'GeV'
        
        hist_kwargs_overwrite = {}
        for label, category in label_2_category.items():
            if signal_categories is None or not category in signal_categories:
                hist_kwargs_overwrite[category] = deepcopy(bkg_hist_kwargs)
            else:
                hist_kwargs_overwrite[category] = {}
            
            hist_kwargs_overwrite[category]['color'] = context.getColorByKey(category)

        plot_kwargs_base = {
            'yscale': 'linear',
            'plot_hist_kwargs': {
                'stacked': False,
                'show_stats': False,
                'normalize': True,
                'hist_kwargs': { 'hatch': None },
                'figsize': (5, 4)
            },
            'ild_style_kwargs': {
                'legend_kwargs': { 'loc': 'upper right', 'bbox_to_anchor': (.98, .98), 'fancybox': False },
                'labelsize': 15,
                'ild_offset_beamspec_mult': 1.05
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
            
            if 'xunit' not in plot_kwargs:
                plot_kwargs['xunit'] = xunits[i]
            
            fig = plot_weighted_hist(plot_dict, title=None, plot_context=context,
                                     plot_hist_kwargs_overwrite=hist_kwargs_overwrite, **plot_kwargs)
            fig.set_tight_layout(True)
            
            figures.append(fig)
            
            plt.close(fig)
            
        if filename is not None:
            export_figures(filename, figures)