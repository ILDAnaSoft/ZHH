from .AnalysisChannel import AnalysisChannel
from .Cuts import Cut
from ..util.PlotContext import PlotContext
from typing import Sequence
from phc import export_figures
from copy import deepcopy
import matplotlib.pyplot as plt
import numpy as np
import os.path as osp

class PreselectionProcessor:
    def __init__(self, sources:list[AnalysisChannel], cuts:Sequence[Cut]|None=None, colormap=None, plot_options:dict[str, list[dict]]|None=None, hypothesis:str='llbbbb'):
        from zhh import colormap_desy, zhh_cuts, preselection_plot_options
        
        assert(len(sources) > 0)
        self._sources = sources
        
        # make sure weighted lumi is the same across all sources
        for source in self._sources:
            if source.getIntegratedLuminosity() != self._sources[0].getIntegratedLuminosity():
                raise ValueError(f"All sources must have the same luminosity. Found {source.getIntegratedLuminosity()} and {self._sources[0].getIntegratedLuminosity()}.")
        
        self._luminosity = self._sources[0].getIntegratedLuminosity()
        self._cuts = cuts if cuts is not None else zhh_cuts('ll')
        self._plot_options:dict[str, list[dict]] = plot_options if plot_options is not None else preselection_plot_options
        self._hypothesis = hypothesis
        
        if True:
            colormap = colormap_desy
        else:
            import matplotlib as mpl
            colormap = mpl.colormaps['viridis'].resampled(20) # if full, 20 is needed
        
        # process():
        self._signal_categories:list[int]|None = None
        self._masks:list[list[tuple[str, np.ndarray]]]|None = None
        self._calc_dicts:list[dict[str, tuple[np.ndarray, np.ndarray]]]|None = None
        self._max_before:np.ndarray|None = None # maximum weighted event count accross all categories before cut
        
        # cutflowPlots()
        self._cmap = colormap
        self._plot_context:PlotContext|None = None
        
        # cutflowTable()

    def process(self, signal_categories:list[int]):
        """Processes the preselection cuts and stores masks for each event in each source passing each cut in _masks.
        Also stores for each passing event the quantity (of the associated cut) and weight in _calc_dicts.
        Masks are AFTER each cut, calc_dicts BEFORE each cut respectively.
        
        Args:
            signal_categories (list[int]): list of process categories, see ProcessCategories. Used for plotting later.       

        Returns:
            _type_: _description_
        """
        
        assert(len(signal_categories) > 0)
        self._signal_categories = signal_categories
        
        from zhh import EventCategories, calc_preselection_by_event_categories
        
        for source in self._sources:
            source.getPreselection().resetView()
                
        # apply cuts
        masks = []
        calc_dicts = []

        def sorting_key(x):
            id, name, values, weights = x
            
            if id in signal_categories:
                return (10**(12 + signal_categories.index(id)))
            else:
                return -np.sum(weights)

        subsets = {}
        for source in self._sources:
            source.getPreselection().resetView()
            subsets[source.getName()] = source.getPreselection()

        max_before_all = []

        for i, cut in enumerate(self._cuts):
            flattened = []
            masks_current_cut = []
            
            max_before = 0
            
            for source in self._sources:
                source_name = source.getName()
                subset = subsets[source_name]
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
        
        self._masks = masks
        self._calc_dicts = calc_dicts
        self._max_before = np.array(max_before_all)
        
        return masks, subsets, calc_dicts
    
    def getFinalEventMasks(self)->list[tuple[str, np.ndarray]]:
        """Returns the masks for the preselection cuts.

        Returns:
            list[tuple[str, np.ndarray]]: list of tuples[name, mask] after the last cut
        """
        if self._masks is None:
            raise RuntimeError("Preselection not processed yet. Please call process() first.")
        
        return self._masks[-1]
    
    def cutflowPlots(self):
        """Creates plots for signal/background separation
        after each cut. When return_last_calc_dict is True, a separate plot showing the event
        count per category after the last cut is created. Uses signal_categories (list[int])
        given to process().
        """
        
        result = cutflowPlots(self)
        self._plot_context = result[0]
        
        return result
        
    
    def cutflowTable(self, final_state_labels_and_names_or_processes:list[tuple[str,str]|tuple[str,str,str]]):
        return cutflowTable(self, final_state_labels_and_names_or_processes)
    
    def getPlotContext(self)->PlotContext:
        """Returns the plot context for the preselection processor.

        Returns:
            PlotContext: The plot context.
        """
        if self._plot_context is None:
            raise RuntimeError("Plot context is not set. Please call process() first.")
        
        return self._plot_context

def cutflowPlots(pp:PreselectionProcessor):
    assert(pp._signal_categories is not None and pp._masks is not None and pp._calc_dicts is not None and pp._max_before is not None)
    
    return cutflowPlotsFn(pp._signal_categories,
        pp._cuts,
        pp._calc_dicts,
        pp._max_before,
        pp._hypothesis,
        pp._cmap,
        pp._plot_options)

def cutflowPlotsFn(signal_categories:list[int],
                 cuts:Sequence[Cut],
                 calc_dicts:list[dict[str, tuple[np.ndarray, np.ndarray]]],
                 max_before:np.ndarray,
                 hypothesis:str,
                 cmap,
                 plot_options:dict[str, list[dict]]):
    
    from zhh import plot_preselection_by_calc_dict, annotate_cut, EventCategories, deepmerge
    
    figs_stacked = []
    figs_sigvbkg = []
        
    context = PlotContext(cmap)
    signal_category_names = [EventCategories.inverted[cat] for cat in signal_categories]
    
    for i, cut in enumerate(cuts[:1]):
        calc_dict = calc_dicts[i]
        max_before = max_before[i]
        
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
        
        for j, key in enumerate(list(set(calc_dict.keys()) - set(signal_category_names))[:9]):    
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
    
    export_figures('cuts.pdf', figs_stacked)
    export_figures('cuts_separate.pdf', figs_sigvbkg)
    
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
    
    export_figures('after_preselection.pdf', [fig])
    
    return context, figs_stacked, figs_sigvbkg, fig

def cutflowTable(pp:PreselectionProcessor, final_state_labels_and_names_or_processes:list[tuple[str,str]|tuple[str,str,str]]):
    assert(pp._masks is not None and pp._luminosity is not None and pp._sources is not None)
    
    return cutflowTableFn(pp._masks,
                        pp._luminosity,
                        pp._sources,
                        final_state_labels_and_names_or_processes,
                        pp._cuts)

def cutflowTableFn(masks,
                 luminosity:float,
                 sources:list[AnalysisChannel],
                 final_state_labels_and_names_or_processes:list[tuple[str,str]|tuple[str,str,str]],
                 cuts:Sequence[Cut]):
    
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
            
            presel = analysis.getPreselection()
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
                        presel = analysis.getPreselection()
                        cut_counts[i_cut] += np.sum(presel['weight'][mask & category_mask])
                        
            print(label, n_expected, cut_counts)
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

        render_latex(latex_out, f"{osp.abspath('')}/llHH_{'efficiency' if calc_cut_efficiency else 'abs'}.pdf")

# for table
def format_counts(x:float):
    return rf'${x:.2E}$'.replace('E+0', r'\cdot 10^') if x > 999 else f'{x:.3g}'
            
def transpose(columns)->list:
    return list(map(list, zip(*columns)))

def calc_cross_section(analysis:AnalysisChannel, process:str):
    from zhh import combined_cross_section
    return combined_cross_section(analysis.getProcesses(), process)