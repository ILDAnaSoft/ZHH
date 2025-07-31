import numpy as np
import matplotlib.pyplot as plt
from .CutflowProcessor import CutflowProcessor
from .AnalysisChannel import AnalysisChannel
from copy import deepcopy
from tqdm.auto import tqdm

class DataExtractor:
    def __init__(self,
                 cutflow_processor:CutflowProcessor,
                 sources:list[AnalysisChannel]|None=None):
        
        self._cp = cutflow_processor
        self._sources = sources if sources is not None else cutflow_processor._sources
        self._features:list[str]|None = None
        
    def extract(self,
                to_process:list[tuple[str, int]],
                features:list[str],
                MOD_WEIGHT:bool=True,
                step:int|None=None)->tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
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

        Raises:
            Exception: _description_

        Returns:
            tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
                src_idx, event_num, labels, weight, inputs
        """
        
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
            nrows_tot += int(mask.sum())
            
            events_passed[fs_name] = mask            

        src_idx = np.zeros(nrows_tot, dtype='B')
        event_num = np.zeros(nrows_tot, dtype='I')
        
        inputs = np.zeros((nrows_tot, len(features)))
        weight = np.zeros(nrows_tot)
        labels = np.zeros(nrows_tot, dtype='B')

        pointer = 0
        pbar = tqdm(range(nrows_tot * len(features)))
        
        for fs_name, class_label in to_process:
            source = fs_2_source[fs_name]
            store = source.getStore()
            
            mask = events_passed[fs_name]
            nrows = int(mask.sum())
            
            src_idx[pointer:pointer + nrows] = src_2_src_idx[source]
            event_num[pointer:pointer + nrows] = store['event'][mask]

            for i, feature in enumerate(features):
                data = store[feature][mask]
                
                pbar.update(len(data))
                pbar.set_description(f'Processing <{source.getName()}.{fs_name}> <{feature}>')
                
                inputs[pointer:pointer + nrows, i] = data
            
            #weight[pointer:pointer + nrows] = (1/nrows * (1 if fs_name == 'µµbb' else 10)) if MOD_WEIGHT else store['weight'][mask]
            weight[pointer:pointer + nrows] = 1/nrows if MOD_WEIGHT else store['weight'][mask]
            labels[pointer:pointer + nrows] = class_label
            
            pointer += nrows
            
        return src_idx, event_num, labels, weight, inputs
    
    def plot(self, inputs, weight, labels):
        assert(self._features is not None)
        
        features = self._features
        
        from zhh import PlotContext, colormap_desy, plot_weighted_hist
        from phc import export_figures
        
        context = PlotContext(colormap_desy)

        xunits:list[None|str] = [None] * len(features)
        xunits[0] = 'GeV'

        hist_kwargs_overwrite = {
            'Signal'    : {  },
            'Background': { 'histtype': 'stepfilled', 'color': context.getColorByKey('Background') }
        }
        plot_kwargs = {
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
            pbar.set_description(f'Processing {feature}')
            
            plot_dict = {}
            plot_dict['Signal']     = ( inputs[labels == 1, i], weight[labels == 1] ) 
            plot_dict['Background'] = ( inputs[labels == 0, i], weight[labels == 0] ) 
            
            fig = plot_weighted_hist(plot_dict, title=f'MVA Input <{feature}>', xlabel=feature, xunit=xunits[i],
                                        plot_context=context, plot_hist_kwargs_overwrite=hist_kwargs_overwrite, **deepcopy(plot_kwargs))
            
            figures.append(fig)
            
            plt.close(fig)
            
        export_figures('mva_inputs.pdf', figures)