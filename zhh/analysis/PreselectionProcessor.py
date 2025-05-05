from .AnalysisChannel import AnalysisChannel
from .Cuts import Cut
from phc import export_figures
from copy import deepcopy
import matplotlib.pyplot as plt
import numpy as np

class PreselectionProcessor:
    def __init__(self, sources:list[AnalysisChannel], cuts:list[Cut]|None=None, colormap=None, plot_options:dict[str, dict]|None=None, hypothesis:str='llbbbb'):
        from zhh import colormap_desy, zhh_cuts, preselection_plot_options
        
        self._sources = sources
        self._cuts = cuts if cuts is not None else zhh_cuts('ll')
        self._plot_options = plot_options if plot_options is not None else preselection_plot_options
        self._hypothesis = hypothesis
        
        if True:
            colormap = colormap_desy
        else:
            import matplotlib as mpl
            colormap = mpl.colormaps['viridis'].resampled(20) # if full, 20 is needed
            
        self._cmap = colormap

    def process(self, signal_categories:list[int], create_plots:bool=True, return_calc_dicts:bool=False):
        """Processes the preselection cuts and creates plots for signal/background separation.

        Args:
            signal_categories (list[int]): list of process categories, see ProcessCategories
            create_plots (bool, optional): _description_. Defaults to True.
            return_calc_dicts (bool, optional): _description_. Defaults to False.

        Returns:
            _type_: _description_
        """
        from zhh import PlotContext, plot_preselection_by_calc_dict, annotate_cut, EventCategories, \
            weighted_counts_by_categories, calc_preselection_by_event_categories, deepmerge
        
        for source in self._sources:
            source.getPreselection().resetView()
                
        # apply cuts
        masks = []
        calc_dicts = []
        
        figs_stacked = []
        figs_sigvbkg = []        
            
        context = PlotContext(self._cmap)
        signal_category_names = [EventCategories.inverted[cat] for cat in signal_categories]

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
            
            selected_sorted = flattened
            selected_sorted = sorted(selected_sorted, key=sorting_key)
            
            # construct the final calc dict
            calc_dict = {}
            for id, name, values, weights in reversed(selected_sorted):
                calc_dict[name] = (values, weights)
                
            if return_calc_dicts:
                calc_dicts.append(calc_dict)
                
            masks.append(masks_current_cut)
            
            if create_plots:
                plot_kwargs = {
                    'bins': 100,
                    'xlabel': cut.quantity,
                    'yscale': 'log',
                    'ild_style_kwargs': {},
                    'plot_hist_kwargs': {},
                    'plot_context': context
                } | deepcopy(self._plot_options[self._hypothesis][i])
                
                plot_kwargs['ild_style_kwargs']['title_postfix'] = rf' before cut on ${cut.formula(unit=plot_kwargs["xunit"] if ("xunit" in plot_kwargs) else None)}$'
                
                # stacked plot
                fig1 = plot_preselection_by_calc_dict(calc_dict, hypothesis=self._hypothesis, **plot_kwargs);
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
                        'ild_style_kwargs': { 'title': key },
                        'plot_hist_kwargs': {
                            'stacked': False,
                            'show_stats': False,
                            'ylim': [1e-3, max_before*1.5],
                            'custom_styling': False,
                            'hist_kwargs': { 'hatch': None }
                        },
                        'plot_context': context,
                    }, deepcopy(self._plot_options[self._hypothesis][i]))
                    
                    plot_dict = { key: calc_dict[key] }
                    for sig_key in signal_category_names:
                        plot_dict[sig_key] = calc_dict[sig_key]
                        
                    #for blabla in plot_dict:
                    #    print('shapes v2: ', blabla, plot_dict[blabla][0].shape, plot_dict[blabla][1].shape)

                    plot_preselection_by_calc_dict(plot_dict, hypothesis=self._hypothesis, plot_hist_kwargs_overwrite=hist_kwargs_overwrite, **plot_kwargs);
                
                figs_sigvbkg.append(fig2)
        
        if create_plots:        
            export_figures('cuts.pdf', figs_stacked)
            export_figures('cuts_separate.pdf', figs_sigvbkg)
        
        if not return_calc_dicts:
            return masks, subsets
        else:
            return masks, subsets, calc_dicts