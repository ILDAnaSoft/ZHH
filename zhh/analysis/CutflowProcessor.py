from .DataSource import DataSource
from .Cuts import Cut, ValueCut
from ..util.PlotContext import PlotContext
from typing import Sequence, Any
from phc import export_figures
from copy import deepcopy
from .TTreeInterface import TTreeInterface
from ..util.deepmerge import deepmerge
import matplotlib.pyplot as plt
from matplotlib.colors import Colormap
import numpy as np
import os.path as osp
import lzma, pickle

def find_entry(entries:list[tuple[str, Any]], name:str):
    for entry in entries:
        if entry[0] == name:
            return entry
        
    raise Exception(f'Entry <{name}> not found')

class CutflowProcessor:
    def __init__(self,
                 sources:list[DataSource],
                 hypothesis:str,
                 signal_categories:list[int],
                 cuts:Sequence[ValueCut]|None=None,
                 colormap:Colormap|None=None,
                 plot_options:dict[str, dict[str, dict]]|None=None,
                 plot_context:PlotContext|None=None,
                 work_dir:str|None=None
    ):
        """Class used to combine multiple DataSource items and process
        cuts over all data. Can be used for preselection analysis and final
        event selection after MVA outputs are attached. See the process method
        for more information.

        Args:
            sources (list[DataSource]): _description_
            hypothesis (str): _description_
            signal_categories (list[int]): list of process categories, see ProcessCategories. Used for plotting later. 
            cuts (Sequence[ValueCut] | None, optional): _description_. Defaults to None.
            colormap (_type_, optional): Cuts used for preselection. Defaults to None.
            plot_options (dict[str, dict[str, dict]] | None, optional): _description_. Defaults to None.
            work_dir (str | None, optional): _description_. Defaults to None.

        Raises:
            ValueError: _description_
        """
        
        from zhh import zhh_cuts, figure_options
        
        assert(len(sources) > 0)
        self._sources = sources
        
        assert(isinstance(signal_categories, list) and len(signal_categories) > 0)
        
        # make sure weighted lumi is the same across all sources
        for source in self._sources:
            if source.getIntegratedLuminosity() != self._sources[0].getIntegratedLuminosity():
                raise ValueError(f"All sources must have the same luminosity. Found {source.getIntegratedLuminosity()} and {self._sources[0].getIntegratedLuminosity()}.")
        
        if work_dir is None:
            work_dir = osp.abspath('')
            
        self._work_dir = work_dir
        self._luminosity = self._sources[0].getIntegratedLuminosity()
        self._cuts:dict[int, Sequence[ValueCut]] = { 0: zhh_cuts(hypothesis) if cuts is None else cuts }
        self._plot_options:dict[str, dict[str, dict]] = plot_options if plot_options is not None else figure_options
        self._hypothesis = hypothesis
        
        if colormap is None:
            if True:
                from zhh import colormap_desy
                colormap = colormap_desy
            else:
                import matplotlib as mpl
                colormap = mpl.colormaps['viridis'].resampled(20) # if full, 20 is needed
        
        # process():
        self._signal_categories = signal_categories
        self._masks:dict[int, list[list[tuple[str, np.ndarray]]]] = {}
        self._calc_dicts:dict[int, list[dict[str, tuple[np.ndarray, np.ndarray]]]] = {}
        self._max_before:dict[int, np.ndarray] = {} # maximum weighted event count accross all categories before cut
        
        # cutflowPlots()
        plot_context = plot_context if plot_context is not None else PlotContext(colormap)
        for sig_cat in signal_categories:
            from zhh import EventCategories
            plot_context.getColorByKey(EventCategories.inverted[sig_cat])
        
        self._plot_context:PlotContext = plot_context
        
        # cutflowTable()

    def getSource(self, name:str)->DataSource:
        for source in self._sources:
            if source.getName() == name:
                return source
        
        raise Exception(f'Source <{name}> not found')

    def sorting_key(self, x):
        id, name, values, weights = x
        
        if id in self._signal_categories:
            return (10**(12 + self._signal_categories.index(id)))
        else:
            return -np.sum(weights)

    def process(self, step:int=0, cuts:Sequence[ValueCut]|None=None,
                weight_prop:str='weight', split:int|None=None,
                cache:str|None='cutflow_presel.pickle'):
        """Processes the preselection cuts and stores masks for each event in
        each source passing each cut in _masks. Also stores for each passing
        event the quantity (of the associated cut) and weight in _calc_dicts.
        For each cut, a mask for AFTER the cut is stored, and an item is for
        calc_dicts is added BEFORE applying the cut. The step property is 0
        for the preselection. To analyze following cuts, supply an incrementing
        integer.
        
        Args: 
            step (int, optional): n-th cut group. Use 0 for preselection,
                1 for the first post-preselection cut group, etc.
                Defaults to 0.
            cuts (Sequence[Cut], optional): Defaults to None.
            weight_prop (str): Defaults to weight.
            split (int, optional): Cut on the split column. Will be ignored
                if None. Defaults to None.
            cache (str, optional): If a string, will be used as filepath to
                                    cache the result of the cut in LZMA com-
                                    pressed pickle format. If None, no file
                                    will be read (use this to study different
                                    cut values) 

        Returns:
            _type_: _description_
        """
        
        from zhh import EventCategories, calc_preselection_by_event_categories
                
        # apply cuts
        masks = []
        calc_dicts = []
        
        if step > 0:
            if not isinstance(cuts, list):
                raise Exception('Using step > 0 requires additional cuts to be supplied')
            
            self._cuts[step] = cuts

        subsets = {}
        for source in self._sources:
            source.getStore().resetView()
            subsets[source.getName()] = source.getStore()

        max_before_all = []
        
        if (step > 0) or cache is None or not osp.isfile(cache):
            
            for i, cut in enumerate(self._cuts[step]):
                flattened = []
                masks_current_cut = []
                
                max_before = 0
                
                for source in self._sources:
                    source_name = source.getName()
                    subset = subsets[source_name]
                    
                    # for post-preselection cuts, use the very last subset as base
                    if i == 0 and step > 0:
                        if not (step - 1) in self._masks:
                            raise Exception('step > 0 requires previous cuts to have been applied already')
                        
                        prev_mask = find_entry(self._masks[step - 1][-1], source_name)[1]
                        
                        # also apply a split, if one is given
                        if split is not None:
                            prev_mask = prev_mask & (subset['split'] == split)

                        subsets[source_name] = subset[prev_mask]
                        subset = subsets[source_name]
                    
                    processes = source.getProcesses()
                    
                    mask = cut(subset)
                    
                    before = np.sum(subset[weight_prop])
                    max_before = max(max_before, before)
                    after = np.sum(subset[weight_prop][mask])
                    
                    #categories, counts = weighted_counts_by_categories(subset)
                    calc_dict = calc_preselection_by_event_categories(subset, processes, quantity=cut.quantity, categories_additional=None, weight_prop=weight_prop)
                    
                    for name in calc_dict:
                        values, weights = calc_dict[name]
                        assert(len(values) == len(weights))
                        
                        flattened.append((getattr(EventCategories, name), name, values, weights))
                        
                    print(f'{source_name} Processing {cut} before: {before} after {after} ({(after/before):.1%})')
                    
                    subsets[source_name] = subset[mask]
                    masks_current_cut.append((source_name, subset._mask))
                
                max_before_all.append(max_before)
                selected_sorted = flattened
                selected_sorted = sorted(selected_sorted, key=self.sorting_key)
                
                # construct the final calc dict
                calc_dict = {}
                for id, name, values, weights in reversed(selected_sorted):
                    calc_dict[name] = (values, weights)
                
                masks.append(masks_current_cut)
                calc_dicts.append(calc_dict)
            
            self._masks[step] = masks
            self._calc_dicts[step] = calc_dicts
            self._max_before[step] = np.array(max_before_all)
            
            if cache is not None and step == 0:
                print(f'Storing preselection in cache at {cache}')
                with lzma.open(cache, 'wb') as f:
                    pickle.dump({
                        'masks': masks,
                        'calc_dicts': calc_dicts,
                        'max_before': self._max_before[step]
                    }, f)
        else:
            print('Using cached preselection')
            with lzma.open(cache, 'r') as f:
                data = pickle.load(f)
                
                self._masks[step] = data['masks']
                self._calc_dicts[step] = data['calc_dicts']
                self._max_before[step] = data['max_before']
        
        return masks, subsets, calc_dicts
    
    def getFinalEventMasks(self, step:int|None=None)->list[tuple[str, np.ndarray]]:
        """Returns the masks for the preselection cuts.
        
        Args: 
            step (int|none, optional): n-th cut group. Use 0 for preselection,
                1 for the first post-preselection cut group, etc. If None,
                the last available cut group will be used.
                Defaults to None.

        Returns:
            list[tuple[str, np.ndarray]]: list of tuples[name, mask] after
            the last cut
        """
        
        if step is None:
            step = len(self._masks)-1
        
        if not step in self._masks:
            raise RuntimeError(f'Cut group <{step} ({"Preselection" if step == 0 else "Post-Preselection/Custom"})> not processed yet. Please call process() first.')
        
        return self._masks[step][-1]
    
    def getFinalEventMaskByName(self, source_name:str, step:int|None=None)->np.ndarray:
        """Return the post-preselection event mask for a given source name
        after the n-th cut group given by step.

        Args:
            source_name (str): name of the source
            step (int|none, optional): n-th cut group. See getFinalEventMasks()

        Raises:
            ValueError: if source name not found in preselection masks

        Returns:
            np.ndarray: the post-preselection event mask for the given source name
        """
        post_presel_mask = None
        
        for name, mask in self.getFinalEventMasks(step=step):
            if name == source_name:
                post_presel_mask = mask
        
        if post_presel_mask is None:
            raise ValueError(f"Source {source_name} not found in preselection masks.")
        
        return post_presel_mask
    
    def writeROOTFiles(self):
        import uproot as ur
        
        with ur.recreate(f'{self._work_dir}/preselection.root', compression=ur.ZSTD(5)) as rf:
            for source in self._sources:
                name = source.getName()
                
                source.getStore().resetView()
            
                data = {
                    'passed_preselection': self.getFinalEventMaskByName(name),
                    'event_weight': source.getStore()['weight'],
                    'event_category': source.getStore()['event_category'],
                }
                    
                rf[name] = data
    
    def cutflowPlots(self, step_start:int=0, step_end:int|None=None, display:bool=True,
                    file:str|None='cutflow_plots.pdf',
                    hist_kwargs:dict={}, plot_options_call:dict[str, dict]={},
                    signal_categories:list[str]|None=None, bins:int|np.ndarray=100,
                    annotate_cut:bool=True):
        """Creates plots for signal/background separation
        after each cut. When return_last_calc_dict is True, a separate plot
        showing the event count per category after the last cut is created.
        Creates multiple PDF files with the plots.
        
        Args:
            step_start (int, optional): First cut group to consider.
                Defaults to 0.
            step_end (int, optional): Last cut group to consider. If None, will
                be set to the last cut group, such that all cuts are considered.
                Defaults to None.
            display (bool, optional): whether to display the created plots.
                Defaults to True.
            file (str): file in which all plots to store
            hist_kwargs (dict, optional): _description_. Defaults to {}.
            signal_categories (list[str] | None, optional): _description_.
                Defaults to None.

        Returns:
            _type_: _description_
        """
        
        context_and_figures = cutflowPlots(self, f'{self._work_dir}/{file}', display,
                              step_start=step_start,
                              step_end=step_end if step_end is not None else len(self._cuts)-1,
                              hist_kwargs=hist_kwargs, plot_options_call=plot_options_call,
                              signal_categories=signal_categories,
                              bins=bins, annotate_cut=annotate_cut)
        
        return context_and_figures
        
    def cutflowTable(self, final_state_labels_and_names_or_processes:list[tuple[str,str]|tuple[str,str,str]|tuple[str,list[str]]],
                     path:str|None=None, step_start:int=0, step_end:int|None=None, weight_prop:str='weight',
                     filename:str|None=None, signal_categories:list[str]=[], ignore_categories:list[str]=[]):
        
        """Creates a cutflow table considering all cuts from groups
        step_start until step_end.

        Args:
            final_state_labels_and_names_or_processes (list[tuple[str,str]|
                tuple[str,str,str]|tuple[str,list[str]]]): _description_
            path (str | None, optional): PDF Table full path. Defaults to None.
            step_start (int, optional): First cut group to consider.
                Defaults to 0.
            step_end (int, optional): Last cut group to consider. If None, will
                be set to the last cut group, such that all cuts are considered.
                Defaults to None.
            weight_prop (float, optional): Which property to use for weights
            filename (): PDF table filename. Will be used together with work_dir
                if path is None. If None, will be set to cutflow_{self._hypothesis}.pdf.
                Defaults to None.

        Returns:
            _type_: _description_
        """
        
        if path is None:
            if filename is None:
                filename = f'cutflow_{self._hypothesis}.pdf'
            
            path = f'{self._work_dir}/{filename}'
            
        return cutflowTable(self, final_state_labels_and_names_or_processes, path,
                            step_start=step_start,
                            step_end=step_end if step_end is not None else
                            len(self._cuts)-1,
                            signal_categories=signal_categories, ignore_categories=ignore_categories, weight_prop=weight_prop)
    
    def getPlotContext(self)->PlotContext:
        """Returns the plot context for the preselection processor.

        Returns:
            PlotContext: The plot context.
        """
        if self._plot_context is None:
            raise RuntimeError("Plot context is not set. Please call process() first.")
        
        return self._plot_context
    
    def _flattenSteps(self, step_end:int, step_start:int=0):
        """Concatenated masks, cuts, calc_dicts, and max_before accross
        multiple cut groups given by step_start and step_end. 

        Args:
            step_end (int): _description_
            step_start (int, optional): _description_. Defaults to 0.

        Returns:
            _type_: _description_
        """
        
        assert(step_start in self._masks and step_start in self._calc_dicts and step_start in self._max_before and
               step_end in self._masks and step_end in self._calc_dicts and step_end in self._max_before)
        
        masks:list[list[tuple[str, np.ndarray]]] = []
        cuts:Sequence[ValueCut] = []
        calc_dicts:list[dict[str, tuple[np.ndarray, np.ndarray]]] = []
        max_before = []
        
        for step in range(step_start, step_end+1):
            masks.extend(self._masks[step])
            cuts.extend(self._cuts[step])
            calc_dicts.extend(self._calc_dicts[step])
            max_before.extend(self._max_before[step])
        
        max_before = np.array(max_before)
        
        return masks, cuts, calc_dicts, max_before
    
    def getSubsetsAt(self, step:int|None=None, split:int|None=None)->dict[str, TTreeInterface]:
        if step == None or step == -1:
            step = len(self._masks)
        
        subsets = {}
        for source in self._sources:
            store = source.getStore()
            store.resetView()
            subsets[source.getName()] = store

        for source_name, subset in subsets.items():  
            if step > 0:
                if not (step - 1) in self._masks:
                    raise Exception('step > 0 requires previous cuts to have been applied already')
                
                prev_mask = find_entry(self._masks[step - 1][-1], source_name)[1]
                
                # also apply a split, if one is given
                if split is not None:
                    prev_mask = prev_mask & (subset['split'] == split)

                subsets[source_name] = subset[prev_mask]
        
        return subsets
        
    def getCalcDictAt(self, quantity:str, step:int|None=None, split:int|None=None, weight_prop:str='weight'):
        from zhh import EventCategories, calc_preselection_by_event_categories
        
        subsets = self.getSubsetsAt(step=step, split=split)
        flattened = []
        
        for src_name, subset in subsets.items():        
            source = self.getSource(src_name)
            processes = source.getProcesses()
            
            calc_dict = calc_preselection_by_event_categories(subset, processes, quantity=quantity, categories_additional=None, weight_prop=weight_prop)
        
            for name in calc_dict:
                values, weights = calc_dict[name]
                assert(len(values) == len(weights))
                
                flattened.append((getattr(EventCategories, name), name, values, weights))

        selected_sorted = flattened
        selected_sorted = sorted(selected_sorted, key=self.sorting_key)

        # construct the final calc dict
        calc_dict = {}
        for id, name, values, weights in reversed(selected_sorted):
            calc_dict[name] = (values, weights)
            
        return calc_dict
            
    def plotAt(self, quantity:str, step:int|None=None, split:int|None=None, plotTop9:bool=False, signal_category_names:list[str]=[], weight_prop:str='weight', **plot_kwargs):
        from zhh import plot_combined_hist

        #plot_kwargs['ild_style_kwargs']['title_postfix'] = rf' before cut on ${cut.formula(unit=plot_kwargs["xunit"] if ("xunit" in plot_kwargs) else None)}$'
        calc_dict = self.getCalcDictAt(quantity, step=step, split=split, weight_prop=weight_prop)

        # stacked plot
        if not plotTop9:
            plot_kwargs_defaults = {
                'bins': 100,
                'xlabel': quantity,
                'yscale': 'log',
                'ild_style_kwargs': {},
                'plot_hist_kwargs': { 'int_bins': False },
                'plot_context': self._plot_context
            }
            plot_kwargs = deepmerge(plot_kwargs_defaults, plot_kwargs)
        
            return plot_combined_hist(calc_dict=calc_dict, hypothesis=self._hypothesis, **plot_kwargs)
        else:
            return plotCalcDictTop9(self.getPlotContext(), calc_dict, quantity, signal_category_names, hypothesis=self._hypothesis, plot_options=plot_kwargs)

def cutflowPlots(cp:CutflowProcessor, output_file:str|None, display:bool=True, step_start:int=0, step_end:int=0,
                 hist_kwargs:dict={}, plot_options_call:dict[str, dict]={}, signal_categories:list[str]|None=None, bins:int|np.ndarray=100,
                 annotate_cut:bool=True):
    
    assert(cp._signal_categories is not None and step_start in cp._masks and step_end in cp._calc_dicts and step_start in cp._max_before)
    
    from zhh import EventCategories
    
    signal_category_names = [EventCategories.inverted[cat] for cat in cp._signal_categories] if signal_categories is None else signal_categories
    
    masks, cuts, calc_dicts, max_before = cp._flattenSteps(step_end, step_start)
    
    return cutflowPlotsFn(signal_category_names,
        cuts,
        calc_dicts,
        cp._hypothesis,
        cp._plot_options,
        plot_options_call,
        display,
        output_file,
        cp._plot_context,
        hist_kwargs,
        bins,
        annotate_cut)

def plotCalcDictTop9(context:PlotContext, calc_dict:dict[str, tuple[np.ndarray, np.ndarray]], quantity:str, signal_category_names:list[str],
                     plot_options:dict={}, hist_kwargs:dict={}, hypothesis:str|None=None, bins:int=100):
    from zhh import plot_combined_hist, deepmerge
    
    fig, axes = plt.subplots(nrows=3, ncols=3, figsize=(18, 18));
    flattened_axes = axes.flatten()

    categories = list(calc_dict.keys())
    for sig_cat in signal_category_names:
        if sig_cat in categories:
            categories.remove(sig_cat)
            
    wt_sum_max = np.max([ cd[1].sum() for cd in calc_dict.values() ])
    xlim_min = np.min([ np.min(cd[0]) for cd in calc_dict.values() ])
    xlim_max = np.max([ np.max(cd[0]) for cd in calc_dict.values() ])

    for j, key in enumerate(list(reversed(categories))[:9]):
        ax = flattened_axes[j]

        hist_kwargs_overwrite = deepcopy(hist_kwargs)
        hist_kwargs_overwrite[key] = { 'color': context.getColorByKey(key), 'histtype': 'stepfilled' }
        
        plot_kwargs = deepmerge({
            'ax': ax,
            'bins': bins,
            'yscale': 'log',
            'ild_style_kwargs': { 'title': key, 'legend_kwargs': { 'loc': 'upper right' } },
            'plot_hist_kwargs': {
                'stacked': False,
                'show_stats': False,
                'ylim': [1e-3, wt_sum_max*1.3],
                'custom_styling': False,
                'hist_kwargs': { 'hatch': None }
            },
            'xlabel': quantity,
            'plot_context': context,
            'hypothesis': hypothesis,
            'xlim': [xlim_min, xlim_max]
        }, deepcopy(plot_options))
        plot_kwargs['ild_style_kwargs']['ild_text_position'] = 'upper left'
        
        if 'hist_kwargs_overwrite' in plot_options:
            deepmerge(hist_kwargs_overwrite, plot_options['hist_kwargs_overwrite'])
        
        plot_dict = { key: calc_dict[key] }
        for sig_key in signal_category_names:
            plot_dict[sig_key] = calc_dict[sig_key]

        fig = plot_combined_hist(plot_dict, plot_hist_kwargs_overwrite=hist_kwargs_overwrite, **plot_kwargs);
        
    return fig

def cutflowPlotsFn(signal_category_names:list[str],
                 cuts:Sequence[ValueCut],
                 calc_dicts:list[dict[str, tuple[np.ndarray, np.ndarray]]],
                 hypothesis:str,
                 plot_options:dict[str, dict[str, dict]],
                 plot_options_call:dict[str, dict[str, dict]],
                 display:bool,
                 output_file:str|None,
                 plot_context:PlotContext,
                 hist_kwargs:dict,
                 bins,
                 do_annotate_cut):
    
    from zhh import plot_combined_hist, annotate_cut, EventCategories, deepmerge
    
    figs_stacked = []
    figs_sigvbkg = []
    
    for i, cut in enumerate(cuts):
        calc_dict = calc_dicts[i]
        
        plot_options_quantity = {}
        if hypothesis in plot_options and cut.quantity in plot_options[hypothesis]:
            plot_options_quantity = deepcopy(plot_options[hypothesis][cut.quantity])
        elif 'default' in plot_options and cut.quantity in plot_options['default']:
            plot_options_quantity = deepcopy(plot_options['default'][cut.quantity])
            
        if cut.quantity in plot_options_call:
            plot_options_quantity = deepmerge(plot_options_quantity, plot_options_call[cut.quantity])
        
        plot_kwargs = {
            'bins': bins,
            'xlabel': cut.label, #rf'${cut.quantity}$',
            'yscale': 'log',
            'ild_style_kwargs': {},
            'plot_hist_kwargs': {}, # hist_kwargs
            'hypothesis': hypothesis,
            'signal_keys': signal_category_names
        }
        plot_kwargs = deepmerge(plot_kwargs, plot_options_quantity)
        plot_kwargs['ild_style_kwargs']['title_postfix'] = rf' before cut on ${cut.formula(unit=plot_kwargs["xunit"] if ("xunit" in plot_kwargs) else None)}$'
        
        if cut.xlim_view is not None:
            plot_kwargs['xlim'] = cut.xlim_view
        
        # stacked plot
        fig1 = plot_combined_hist(calc_dict, plot_context=plot_context, **deepcopy(plot_kwargs));
        if do_annotate_cut:
            annotate_cut(fig1.axes[0], cut);
        
        figs_stacked.append(fig1)
        
        # non-stacked plot
        fig2 = plotCalcDictTop9(plot_context, calc_dict, cut.label, signal_category_names, plot_options_quantity, hist_kwargs=hist_kwargs, hypothesis=hypothesis, bins=bins);
        figs_sigvbkg.append(fig2)
        
        if not display:
            plt.close(fig1)
            plt.close(fig2)
    
    # create a plot with the event counts per category after the last cut
    from zhh.plot.ild_style import update_plot, legend_kwargs_fn
    
    counts_final = []
    counts_start = {}
    calc_dict_final = calc_dicts[-1]
    last_cut = cuts[-1]

    for cat in calc_dict_final:    
        last_cut_value = calc_dict_final[cat][0]
        mask = last_cut.raw(last_cut_value)
        counts_final.append((cat, calc_dict_final[cat][1][mask].sum()))
    
    calc_dict_first = calc_dicts[0]
    for cat in calc_dict_first:
        counts_start[cat] = calc_dict_first[cat][1].sum()

    fig, ax = plt.subplots(figsize=(15,6))

    bar_colors = []
    bar_labels = []
    bar_counts = []
    bar_descriptions = []

    for name, count in sorted(counts_final, key=lambda x: x[1]):
        bar_colors.append(plot_context.getColorByKey(name))
        bar_labels.append(name)
        bar_counts.append(count)
        bar_descriptions.append(f'{format_counts(count)} ({(count / counts_start[name]):.1%})')
        
    bar_container = ax.bar(bar_labels, bar_counts, label=bar_labels, color=bar_colors)
    ax.bar_label(bar_container, labels=bar_descriptions, label_type='edge', fontname=plot_context.getFont(), fontsize=9)
    ax.set_yscale('log')
    ax.legend(title='Event categories', loc='upper left', **legend_kwargs_fn(plot_context))
    
    update_plot(ax, x_label=None, y_label='Event count', title='wt. events after selection', context=plot_context)
    
    all_figs = []
    for i in range(len(figs_stacked)):
        all_figs.append(figs_stacked[i])
        all_figs.append(figs_sigvbkg[i])
    all_figs.append(fig)
    
    if output_file is not None:
        export_figures(output_file, all_figs)
    
    if not display:
        plt.close(fig)
    
    return plot_context, figs_stacked, figs_sigvbkg, fig

def cutflowTable(cp:CutflowProcessor, final_state_labels_and_names_or_processes:list[tuple[str,str]|tuple[str,str,str]|tuple[str,list[str]]], path:str,
                 step_start:int, step_end:int, weight_prop:str='weight', signal_categories:list[str]=[], ignore_categories:list[str]=[]):
    
    masks, cuts, calc_dicts, max_before = cp._flattenSteps(step_end, step_start)
    
    return cutflowTableFn(masks,
                        cp._luminosity,
                        cp._sources,
                        final_state_labels_and_names_or_processes,
                        cuts,
                        path,
                        weight_prop,
                        step_start,
                        signal_categories,
                        ignore_categories)

def cutflowTableFn(masks,
                 luminosity:float,
                 sources:list[DataSource],
                 final_state_labels_and_names_or_processes:list[tuple[str,str]|tuple[str,str,str]|tuple[str,list[str]]],
                 cuts:Sequence[Cut],
                 path:str,
                 weight_prop:str,
                 step_start:int,
                 signal_categories:list[str]=[],
                 ignore_categories:list[str]=[]):
    
    from zhh import combined_cross_section, render_table, render_latex, EventCategories
    
    using_split = weight_prop != 'weight'
    
    cross_sections:dict[str, float]  = { 'OTHBKGS': 0. }
    expected_events:dict[str, float] = { 'OTHBKGS': 0. }
    counts_abs:dict[str, np.ndarray] = {}
    first_signal_pos = -1
    is_signal:dict[str, bool] = {}
    source_2_mask:dict[str, dict[str, list]] = {}
    other_bkg_count_tot = np.zeros(len(masks))

    for calc_cut_efficiency in [False, True]: # [False]: # 
        first_column = ['']
        if not using_split:
            first_column.append('expected events')
            
        first_column += [rf"${cut.__repr__().replace('<Cut on ', '')[:-1]}$" for cut in cuts]
        if not calc_cut_efficiency and not using_split:
            first_column.insert(1, r'$\sigma$ [fb]')
        columns = [first_column]

        for j, entry in enumerate(final_state_labels_and_names_or_processes):
            process = None
            if len(entry) == 2:
                label, mask_name = entry
            elif len(entry) == 3:
                label, mask_name, process = entry
            else:
                raise ValueError(f"Entry must be a tuple of length 2 or 3. Found {len(entry)}.")
            
            mask_names = [mask_name] if isinstance(mask_name, str) else mask_name
            
            # find the first occurence of a signal class to know where to place bkg sum
            name_concat = '_'.join(mask_names)
            is_signal[name_concat] = name_concat in signal_categories
                
            if is_signal[name_concat] and first_signal_pos == -1:
                first_signal_pos = j
        
            found = False
            for analysis in sources:
                if analysis.containsFinalState(mask_names[0]):
                    if found:
                        raise ValueError(f"Multiple analyses found containing the final state {mask_name}. Support needs to be checked.")
                    else:
                        found = True
                        if process is not None and not analysis.containsProcess(process):
                            raise ValueError(f"Process {process} not found in analysis {analysis.getName()} but required.")
                        
                        break

            for i in range(len(mask_names) - 1):
                assert(analysis.containsFinalState(mask_names[i + 1]))
            
            if not found:
                raise ValueError(f"No analysis found containing the final state {mask_name}.")
            
            store = analysis.getStore()
            store.resetView()
            category_mask = analysis.getCategoryMask(mask_names[0])
            for i in range(len(mask_names) - 1):
                category_mask = category_mask & analysis.getCategoryMask(mask_names[i + 1])
            
            if not calc_cut_efficiency:                
                cut_counts = np.zeros(len(masks), dtype=float)
                
                for i_cut in range(len(masks)):
                    for source_name, mask in masks[i_cut]:
                        if source_name == analysis.getName():
                            store = analysis.getStore()
                            has_passed_and_in_category = mask & category_mask
                            cut_counts[i_cut] += np.sum(store[weight_prop][has_passed_and_in_category])
                            
                            # keep track of masks accounted for, to treat remaining as background
                            if not source_name in source_2_mask:
                                source_2_mask[source_name] = {}
                            
                            if not name_concat in source_2_mask[source_name]:
                                source_2_mask[source_name][name_concat] = [category_mask, []]
                            
                            source_2_mask[source_name][name_concat][1].append(mask)
                
                counts_abs[name_concat] = cut_counts
                
                if process is not None:
                    cross_sec = combined_cross_section(analysis.getProcesses(), process)
                    n_expected = int(cross_sec * luminosity * 1000)
                else:
                    n_expected = np.sum(store[weight_prop][category_mask])
                    cross_sec = n_expected / (luminosity * 1000)
                
                cross_sections[name_concat] = cross_sec
                expected_events[name_concat] = n_expected
            else:
                # get from abs numbers
                cut_counts = counts_abs[name_concat]
                cross_sec = cross_sections[name_concat]
                n_expected = expected_events[name_concat]
                        
            #print(label, n_expected, cut_counts)
            # finalize entry to write
            entry = [rf'$\mathbf{{ {label} }}$']
            
            if not using_split:                    
                entry.append(format_counts(n_expected))
            
            if calc_cut_efficiency:
                cut_counts_out = np.zeros(cut_counts.shape, dtype=float)
                
                for i_cut in range(len(cut_counts)):
                    nom = cut_counts[i_cut]
                    denom = n_expected if i_cut == 0 else cut_counts[i_cut-1]             
                    #print(name_concat, i_cut, nom/denom, nom, denom)
                    
                    cut_counts_out[i_cut] = nom / denom if (denom != 0) else 0
                    
                cut_counts = cut_counts_out
            elif not using_split:
                entry.insert(1, f'{cross_sec:.3g}')
            
            for j, x in enumerate(cut_counts):
                entry.append(f'{x:.2%}'.replace('%', r'\%') if calc_cut_efficiency else format_counts(x))
            
            #print(f'{name_concat} n={len(entry)}', entry)
            columns.append(entry)
        
        # other backgrounds        
        if not calc_cut_efficiency:  
            for source in sources:
                source_name = source.getName()
                store = source.getStore()
                store.resetView()
                
                other_bkg_count = np.zeros(len(masks))
                other_bkg_mask_total = np.ones(len(store), dtype=bool)
                
                for i_cut in range(len(masks)):
                    has_passed = find_entry(masks[i_cut], source_name)[1]
                    if source_name in source_2_mask:
                        for mask_name, category_mask_and_cut_masks in source_2_mask[source_name].items():
                            category_mask, cut_masks = category_mask_and_cut_masks
                            has_passed_and_in_category = category_mask & cut_masks[i_cut]
                            
                            # count everything that has passed and
                            # that which not included in another mask                        
                            has_passed = has_passed & (~has_passed_and_in_category)
                            
                            # to estimate total count + xsec of non-accounted bkg events
                            if i_cut == 0:
                                other_bkg_mask_total = other_bkg_mask_total & (~category_mask)
                    
                    other_bkg_count[i_cut] = store[weight_prop][has_passed].sum()
                        
                #print(f'{source_name}: other_bkg_count', other_bkg_count)
                other_bkg_count_tot += other_bkg_count
                
                expected_events['OTHBKGS'] += store[weight_prop][other_bkg_mask_total].sum()
            
            cross_sections['OTHBKGS'] += expected_events['OTHBKGS'] / (luminosity * 1000)
            
            #print('other_bkg_count_tot', other_bkg_count_tot)
        
        # add other backgrounds
        if first_signal_pos != -1:
            entry = [r'\textbf{Other bkgs}']
            
            if not using_split:
                if not calc_cut_efficiency:
                    entry.append(format_counts(cross_sections['OTHBKGS']))
                
                entry.append(format_counts(expected_events['OTHBKGS']))
                
            for j, count in enumerate(other_bkg_count_tot):
                nom = count
                denom = (expected_events['OTHBKGS'] if j == 0 else other_bkg_count_tot[j - 1]) if calc_cut_efficiency else 1.
                
                entry.append(f'{(nom/denom):.2%}'.replace('%', r'\%') if calc_cut_efficiency else format_counts(nom / denom))
                
            columns.insert(first_signal_pos + 1, entry)
        
        # add sum of all + other backgrounds
        background_categories = []
        for category, flag in is_signal.items():
            if not flag and not category in ignore_categories:
                background_categories.append(category)
        
        if first_signal_pos != -1:
            entry = [r'\textbf{Total bkg}']
            nrows = len(counts_abs[background_categories[0]])
            counts_bkg = np.zeros(nrows)
            cross_sec_bkg = cross_sections['OTHBKGS'] # 0
            n_expected_bkg = expected_events['OTHBKGS'] # 0
            
            for category in background_categories:
                counts_bkg += counts_abs[category]
                cross_sec_bkg += cross_sections[category]
                n_expected_bkg += expected_events[category]
            
            if not using_split:
                if not calc_cut_efficiency:
                    entry.append(format_counts(cross_sec_bkg))
                
                entry.append(format_counts(n_expected_bkg))
                
            for j, count in enumerate(counts_bkg):
                nom = count + other_bkg_count_tot[j]
                denom = (n_expected_bkg if j == 0 else (counts_bkg[j - 1] + other_bkg_count_tot[j - 1])) if calc_cut_efficiency else 1.
                #print(j, nom / denom, nom, denom)
                
                entry.append(f'{(nom/denom):.2%}'.replace('%', r'\%') if calc_cut_efficiency else format_counts(nom / denom))
                
            columns.insert(first_signal_pos + 2, entry)
            
        table = transpose(columns)
        if not using_split:
            table.insert(2 if calc_cut_efficiency else 3, r'\hline')
        
        if step_start == 0 and not using_split:
            table.insert(5 if calc_cut_efficiency else 6, r'\hline')

        # print(tabulate(table, headers='firstrow', stralign='center', numalign='center', disable_numparse=True)) #tablefmt='latex_raw', ))
        # latex_out = tabulate(table, headers='firstrow', tablefmt='latex_raw', numalign='right', disable_numparse=True)
        latex_out = render_table(table)
        print(latex_out)

        print(osp.splitext(path)[0] +('_efficiency.pdf' if calc_cut_efficiency else '_abs.pdf'))
        render_latex(latex_out, osp.splitext(path)[0] +('_efficiency.pdf' if calc_cut_efficiency else '_abs.pdf'))
        
    #return cross_sections, expected_events, counts_abs, is_signal

# for table
def format_counts(x:float):
    if x > 999:
        return rf'${x:.2E}$'.replace('E+0', r'\cdot 10^')
    elif x > 99:
        return rf'${x:.3g}$'
    else:
        return significant_digits(x) #f'{x:.3g}'
            
def transpose(columns)->list:
    return list(map(list, zip(*columns)))

def calc_cross_section(analysis:DataSource, process:str):
    from zhh import combined_cross_section
    return combined_cross_section(analysis.getProcesses(), process)

def significant_digits(num:float|int, ndigits=3):
    return f'${num:.{ndigits}}$'
