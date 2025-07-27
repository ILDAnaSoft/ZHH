from .AnalysisChannel import AnalysisChannel
from .Cuts import Cut
from ..util.PlotContext import PlotContext
from typing import Sequence, Any
from phc import export_figures
from copy import deepcopy
import matplotlib.pyplot as plt
import numpy as np
import os.path as osp

def find_entry(entries:list[tuple[str, Any]], name:str):
    for entry in entries:
        if entry[0] == name:
            return entry
        
    raise Exception(f'Entry <{name}> not found')

class CutflowProcessor:
    def __init__(self,
                 sources:list[AnalysisChannel],
                 hypothesis:str,
                 signal_categories:list[int],
                 cuts:Sequence[Cut]|None=None,
                 colormap=None,
                 plot_options:dict[str, list[dict]]|None=None,
                 work_dir:str|None=None
    ):
        """Class used to combine multiple AnalysisChannel items and process
        cuts over all data. Can be used for preselection analysis and final
        event selection after MVA outputs are attached. See the process method
        for more information.

        Args:
            sources (list[AnalysisChannel]): _description_
            hypothesis (str): _description_
            signal_categories (list[int]): list of process categories, see ProcessCategories. Used for plotting later. 
            cuts (Sequence[Cut] | None, optional): _description_. Defaults to None.
            colormap (_type_, optional): Cuts used for preselection. Defaults to None.
            plot_options (dict[str, list[dict]] | None, optional): _description_. Defaults to None.
            work_dir (str | None, optional): _description_. Defaults to None.

        Raises:
            ValueError: _description_
        """
        
        from zhh import colormap_desy, zhh_cuts, preselection_plot_options
        
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
        self._cuts:dict[int, Sequence[Cut]] = { 0: zhh_cuts(hypothesis) if cuts is None else cuts }
        self._plot_options:dict[str, list[dict]] = plot_options if plot_options is not None else preselection_plot_options
        self._hypothesis = hypothesis
        
        if True:
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
        self._cmap = colormap
        self._plot_context:PlotContext|None = None
        
        # cutflowTable()

    def process(self, step:int=0, cuts:Sequence[Cut]|None=None):
        """Processes the preselection cuts and stores masks for each event in
        each source passing each cut in _masks. Also stores for each passing
        event the quantity (of the associated cut) and weight in _calc_dicts.
        Masks are AFTER each cut, calc_dicts BEFORE each cut respectively. The
        steop property is 0 for the preselection. To analyze following cuts,
        supply an incrementing integer.
        
        Args: 
            step (int, optional): n-th cut group. Use 0 for preselection,
                1 for the first post-preselection cut group, etc.
                Defaults to 0.
            cuts (Sequence[Cut]|None): Defaults to None.

        Returns:
            _type_: _description_
        """
        
        from zhh import EventCategories, calc_preselection_by_event_categories
                
        # apply cuts
        masks = []
        calc_dicts = []
        
        if step > 0:
            if cuts is None:
                raise Exception('Using step > 0 requires additional cuts to be supplied')
            
            self._cuts[step] = cuts       

        def sorting_key(x):
            id, name, values, weights = x
            
            if id in self._signal_categories:
                return (10**(12 + self._signal_categories.index(id)))
            else:
                return -np.sum(weights)

        subsets = {}
        for source in self._sources:
            source.getData().resetView()
            subsets[source.getName()] = source.getData()

        max_before_all = []

        for i, cut in enumerate(self._cuts[step]):
            flattened = []
            masks_current_cut = []
            
            max_before = 0
            
            for source in self._sources:
                source_name = source.getName()
                subset = subsets[source_name]
                
                # for post-preselection cuts, use the very last subset as base
                if step > 0:
                    if not (step -1) in self._masks:
                        raise Exception('step > 0 requires previous cuts to have been applied already')
                    
                    subsets[source_name] = subset[self._masks[step - 1]]
                
                processes = source.getProcesses()
                
                mask = cut(subset)
                
                before = np.sum(subset['weight'])
                max_before = max(max_before, before)
                after = np.sum(subset['weight'][mask])
                
                #categories, counts = weighted_counts_by_categories(subset)
                calc_dict = calc_preselection_by_event_categories(subset, processes, quantity=cut.quantity, categories_additional=None)
                
                for name in calc_dict:
                    values, weights = calc_dict[name]
                    assert(len(values) == len(weights))
                    
                    flattened.append((getattr(EventCategories, name), name, values, weights))
                    
                print(f"{source_name} Processing {cut} before: {before} after {after} ({(after/before):.1%})")
                
                subsets[source_name] = subset[mask]
                masks_current_cut.append((source_name, subset._mask))
            
            max_before_all.append(max_before)
            selected_sorted = flattened
            selected_sorted = sorted(selected_sorted, key=sorting_key)
            
            # construct the final calc dict
            calc_dict = {}
            for id, name, values, weights in reversed(selected_sorted):
                calc_dict[name] = (values, weights)
            
            masks.append(masks_current_cut)
            calc_dicts.append(calc_dict)
        
        self._masks[step] = masks
        self._calc_dicts[step] = calc_dicts
        self._max_before[step] = np.array(max_before_all)
        
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
                
                source.getData().resetView()
            
                data = {
                    'passed_preselection': self.getFinalEventMaskByName(name),
                    'event_weight': source.getData()['weight'],
                    'event_category': source.getData()['event_category'],
                }
                    
                rf[name] = data
    
    def cutflowPlots(self, step_start:int=0, step_end:int|None=None, display:bool=True):
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
        
        """
        
        result = cutflowPlots(self, self._work_dir, display, step_start=step_start,
                              step_end=step_end if step_end is not None else
                                len(self._cuts)-1)
        self._plot_context = result[0]
        
        return result
        
    def cutflowTable(self, final_state_labels_and_names_or_processes:list[tuple[str,str]|tuple[str,str,str]],
                     path:str|None=None, step_start:int=0, step_end:int|None=None):
        
        """Creates a cutflow table considering all cuts from groups
        step_start until step_end.

        Args:
            final_state_labels_and_names_or_processes (list[tuple[str,str] | tuple[str,str,str]]): _description_
            path (str | None, optional): _description_. Defaults to None.
            step_start (int, optional): First cut group to consider.
                Defaults to 0.
            step_end (int, optional): Last cut group to consider. If None, will
                be set to the last cut group, such that all cuts are considered.
                Defaults to None.

        Returns:
            _type_: _description_
        """
        if path is None:
            path = f'{self._work_dir}/cutflow_{self._hypothesis}.pdf'
            
        return cutflowTable(self, final_state_labels_and_names_or_processes, path,
                            step_start=step_start,
                            step_end=step_end if step_end is not None else
                            len(self._cuts)-1)
    
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
        cuts:Sequence[Cut] = []
        calc_dicts:list[dict[str, tuple[np.ndarray, np.ndarray]]] = []
        max_before = []
        
        print(step_start, step_end+1)
        for step in range(step_start, step_end+1):
            masks.extend(self._masks[step])
            cuts.extend(self._cuts[step])
            calc_dicts.extend(self._calc_dicts[step])
            max_before.extend(self._max_before[step])
            
            print(self._max_before[step])
        
        max_before = np.array(max_before)
        
        return masks, cuts, calc_dicts, max_before

def cutflowPlots(cp:CutflowProcessor, output_dir:str, display:bool=True, step_start:int=0, step_end:int=0):
    assert(cp._signal_categories is not None and step_start in cp._masks and step_end in cp._calc_dicts and step_start in cp._max_before)
    
    masks, cuts, calc_dicts, max_before = cp._flattenSteps(step_end, step_start)
    
    return cutflowPlotsFn(cp._signal_categories,
        cuts,
        calc_dicts,
        max_before,
        cp._hypothesis,
        cp._cmap,
        cp._plot_options,
        display,
        output_dir)

def cutflowPlotsFn(signal_categories:list[int],
                 cuts:Sequence[Cut],
                 calc_dicts:list[dict[str, tuple[np.ndarray, np.ndarray]]],
                 max_before_all:np.ndarray,
                 hypothesis:str,
                 cmap,
                 plot_options:dict[str, list[dict]],
                 display:bool,
                 output_dir:str):
    
    from zhh import plot_preselection_by_calc_dict, annotate_cut, EventCategories, deepmerge
    
    figs_stacked = []
    figs_sigvbkg = []
        
    context = PlotContext(cmap)
    signal_category_names = [EventCategories.inverted[cat] for cat in signal_categories]
    
    for i, cut in enumerate(cuts):
        calc_dict = calc_dicts[i]
        max_before = max_before_all[i]
        
        plot_kwargs = {
            'bins': 100,
            'xlabel': cut.quantity,
            'yscale': 'log',
            'ild_style_kwargs': {},
            'plot_hist_kwargs': {},
            'plot_context': context
        } | deepcopy(plot_options[hypothesis][i])
        
        plot_kwargs['ild_style_kwargs']['title_postfix'] = rf' before cut on ${cut.formula(unit=plot_kwargs["xunit"] if ("xunit" in plot_kwargs) else None)}$'
        
        # stacked plot
        fig1 = plot_preselection_by_calc_dict(calc_dict, hypothesis=hypothesis, **plot_kwargs);
        annotate_cut(fig1.axes[0], cut);
        figs_stacked += [fig1]
        
        # non-stacked plot
        fig2, axes = plt.subplots(nrows=3, ncols=3, figsize=(18, 18))
        flattened_axes = axes.flatten()
        
        categories = list(calc_dict.keys())
        for sig_cat in signal_category_names:
            if sig_cat in categories:
                categories.remove(sig_cat)

        for j, key in enumerate(list(reversed(categories))[:9]):    
            ax = flattened_axes[j]
        
            hist_kwargs_overwrite = {}
            hist_kwargs_overwrite[key] = { 'color': context.getColorByKey(key), 'histtype': 'stepfilled' }
            
            plot_kwargs = deepmerge({
                'ax': ax,
                'bins': 100,
                'xlabel': cut.quantity,
                'yscale': 'log',
                'ild_style_kwargs': { 'title': key, 'legend_kwargs': { 'loc': 'upper right' } },
                'plot_hist_kwargs': {
                    'stacked': False,
                    'show_stats': False,
                    'ylim': [1e-3, max_before*1.5],
                    'custom_styling': False,
                    'hist_kwargs': { 'hatch': None }
                },
                'plot_context': context,
            }, deepcopy(plot_options[hypothesis][i]))
            plot_kwargs['ild_style_kwargs']['ild_text_position'] = 'upper left'
            
            plot_dict = { key: calc_dict[key] }
            for sig_key in signal_category_names:
                plot_dict[sig_key] = calc_dict[sig_key]
                
            #for blabla in plot_dict:
            #    print('shapes v2: ', blabla, plot_dict[blabla][0].shape, plot_dict[blabla][1].shape)

            plot_preselection_by_calc_dict(plot_dict, hypothesis=hypothesis, plot_hist_kwargs_overwrite=hist_kwargs_overwrite, **plot_kwargs);
        
        figs_sigvbkg.append(fig2)
        
        if not display:
            plt.close(fig1)
            plt.close(fig2)
    
    export_figures(f'{output_dir}/cuts.pdf', figs_stacked)
    export_figures(f'{output_dir}/cuts_separate.pdf', figs_sigvbkg)
    
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
        bar_colors.append(context.getColorByKey(name))
        bar_labels.append(name)
        bar_counts.append(count)
        bar_descriptions.append(f'{format_counts(count)} ({(count / counts_start[name]):.1%})')
        
    bar_container = ax.bar(bar_labels, bar_counts, label=bar_labels, color=bar_colors)
    ax.bar_label(bar_container, labels=bar_descriptions, label_type='edge', fontname=context.getFont(), fontsize=9)
    ax.set_yscale('log')
    ax.legend(title='Event categories', loc='upper left', **legend_kwargs_fn(context))
    
    update_plot(ax, x_label=None, y_label='Event count', title='Events after full preselection', context=context)
    
    export_figures(f'{output_dir}/after_preselection.pdf', [fig])
    
    if not display:
        plt.close(fig)
    
    return context, figs_stacked, figs_sigvbkg, fig

def cutflowTable(cp:CutflowProcessor, final_state_labels_and_names_or_processes:list[tuple[str,str]|tuple[str,str,str]], path:str,
                 step_start:int, step_end:int):
    
    masks, cuts, calc_dicts, max_before = cp._flattenSteps(step_end, step_start)
    
    return cutflowTableFn(masks,
                        cp._luminosity,
                        cp._sources,
                        final_state_labels_and_names_or_processes,
                        cuts,
                        path)

def cutflowTableFn(masks,
                 luminosity:float,
                 sources:list[AnalysisChannel],
                 final_state_labels_and_names_or_processes:list[tuple[str,str]|tuple[str,str,str]],
                 cuts:Sequence[Cut],
                 path:str):
    
    from zhh import combined_cross_section, render_table, render_latex

    for calc_cut_efficiency in [False, True]:
        first_column = ['', 'expected events'] + [rf"${cut.__repr__().replace('<Cut on ', '')[:-1]}$" for cut in cuts]
        if not calc_cut_efficiency:
            first_column.insert(1, r'$\sigma$ [fb]')
        columns = [first_column]

        for entry in final_state_labels_and_names_or_processes:
            process = None
            if len(entry) == 2:
                label, mask_name = entry
            elif len(entry) == 3:
                label, mask_name, process = entry
            else:
                raise ValueError(f"Entry must be a tuple of length 2 or 3. Found {len(entry)}.")
            
            found = False
            for analysis in sources:
                if analysis.containsFinalState(mask_name):
                    if found:
                        raise ValueError(f"Multiple analyses found containing the final state {mask_name}. Support needs to be checked.")
                    else:
                        found = True
                        if process is not None and not analysis.containsProcess(process):
                            raise ValueError(f"Process {process} not found in analysis {analysis.getName()} but required.")
                        
                        break
            
            if not found:
                raise ValueError(f"No analysis found containing the final state {mask_name}.")
            
            presel = analysis.getData()
            presel.resetView()
            category_mask = analysis.getCategoryMask(mask_name)
            
            if process is not None:
                cross_sec = combined_cross_section(analysis.getProcesses(), process)
                n_expected = int(cross_sec * luminosity * 1000)
            else:
                n_expected = np.sum(presel['weight'][category_mask])
                cross_sec = n_expected / (luminosity * 1000)
            
            cut_counts = np.zeros(len(masks), dtype=float)
            
            for i_cut in range(len(masks)):
                for source_name, mask in masks[i_cut]:
                    if source_name == analysis.getName():
                        presel = analysis.getData()
                        cut_counts[i_cut] += np.sum(presel['weight'][mask & category_mask])
                        
            #print(label, n_expected, cut_counts)
            # finalize entry to write
            entry = [
                rf'$\mathbf{{ {label} }}$',
                format_counts(n_expected)
            ]
            
            if calc_cut_efficiency:
                cut_counts_out = np.zeros(cut_counts.shape, dtype=float)
                
                for i_cut in range(len(cut_counts)):
                    nom = cut_counts[i_cut]
                    denom = n_expected if i_cut == 0 else cut_counts[i_cut-1]
                    print(nom, denom)
                    
                    cut_counts_out[i_cut] = nom / denom if (denom != 0) else 0
                    
                cut_counts = cut_counts_out
            else:
                entry.insert(1, f'{cross_sec:.3g}')
            
            entry += map(lambda x: f'{x:.2%}'.replace('%', r'\%') if calc_cut_efficiency else format_counts(x), cut_counts)
            columns.append(entry)

        table = transpose(columns)
        table.insert(3, r'\hline')
        table.insert(6, r'\hline')

        # print(tabulate(table, headers='firstrow', stralign='center', numalign='center', disable_numparse=True)) #tablefmt='latex_raw', ))
        # latex_out = tabulate(table, headers='firstrow', tablefmt='latex_raw', numalign='right', disable_numparse=True)
        latex_out = render_table(table)
        print(latex_out)

        render_latex(latex_out, osp.splitext(path)[0] +('_efficiency.pdf' if calc_cut_efficiency else '_abs.pdf'))

# for table
def format_counts(x:float):
    return rf'${x:.2E}$'.replace('E+0', r'\cdot 10^') if x > 999 else f'{x:.3g}'
            
def transpose(columns)->list:
    return list(map(list, zip(*columns)))

def calc_cross_section(analysis:AnalysisChannel, process:str):
    from zhh import combined_cross_section
    return combined_cross_section(analysis.getProcesses(), process)