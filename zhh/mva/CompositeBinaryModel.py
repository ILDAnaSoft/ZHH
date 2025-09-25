import numpy as np
from .MulticlassModel import MulticlassModel
from .MVAModule import MVA_MODULE_STATES, MVAModule
from ..analysis.DataExtractor import DataExtractor
from ..util.PlotContext import PlotContext

class CompositeBinaryModel(MulticlassModel):
    def __init__(self, extractor:DataExtractor, classes:list[tuple[str, int]], clfs:list[MVAModule], threshold_scan:np.ndarray|None=None):        
        self._extractor = extractor
        self._nclasses = len(classes)
        self._classes = classes
        self._clfs = clfs
        self._stats:dict[str, np.ndarray] = {}
        self._threshold_scan = MulticlassModel.threshold_scan() if threshold_scan is None else threshold_scan
        self._optimal_cuts:np.ndarray = np.zeros(len(clfs))
        
    def getStatistics(self):
        assert(len(self._stats))
        return self._stats
    
    def getClasses(self):
        return self._classes
        
    def initialize(self, record_stats:bool=True,
                   plot_features:bool=True,
                   force_retrain_per_clf:list[bool]|None=None,
                   plot_features_bname:str|None=None,
                   plot_options:dict[str, dict]={},
                   cache:str|None='cache',
                   train_kwargs:dict[int, dict]={},
                   plot_context:PlotContext|bool=True):
        """Initialized the whole model. Trains the individuals classifiers (if
        needed) and optionally plots the training input distributions and si-
        gnificance curves using the test dataset.

        Args:
            record_stats (bool, optional): Populates the _stats property with
                efficiency/purity/significance statistics from a threshold
                scan if True. Defaults to True.
            plot_features (bool, optional): _description_. Defaults to True.
            force_retrain_per_clf (list[bool] | None, optional): _description_. Defaults to None.
            plot_features_bname (str | None, optional): _description_. Defaults to None.
            plot_options (dict[str, dict], optional): feature: plot_kwargs. Defaults to {}.
        """
        
        assert(len(self._classes) == (len(self._clfs) + 1))
        
        if force_retrain_per_clf is None:
            force_retrain_per_clf = [False] * len(self._clfs)
        
        if plot_features_bname is None:
            plot_features_bname = 'mva_inputs_$i.pdf'
        
        generate_plot_context = plot_context == True
        use_plot_context_in = not generate_plot_context and isinstance(plot_context, PlotContext)
        
        for i in range(self._nclasses - 1):
            sig_class, bkg_class = self._classes[0][0], self._classes[i + 1][0]
            if not use_plot_context_in:
                if generate_plot_context:
                    plot_context = None
                else:
                    plot_context = self._extractor._cp.getPlotContext()                
            
            clf = self._clfs[i]
            
            print(f'\nProcessing step {i}: {sig_class} vs {bkg_class}')
            
            from os.path import isfile
            
            if not clf.getState() == MVA_MODULE_STATES.READY or plot_features or force_retrain_per_clf[i]:
                # step=0 to force extract after preselection
                # split=0 use first split
                cache_file = f'{cache}_train_class_{i}.npz' if cache is not None else None
                
                if cache_file is not None and isfile(cache_file):
                    cache_npz = np.load(cache_file)
                    train_labels, train_weight, train_inputs, features = cache_npz['train_labels'], cache_npz['train_weight'], cache_npz['train_inputs'], cache_npz['features']
                    features = list(features)
                    
                    self._extractor._features = features
                    self._extractor._labels = np.array([ 0, 1 ], dtype='B')
                    assert(clf.getFeatures() == features)
                else:
                    train_src_idx, train_event_num, \
                    train_labels, train_weight, train_inputs = self._extractor.extract([ (bkg_class, 0), (sig_class, 1) ], clf.getFeatures(), step=0, split=0, weight_prop='weights_split')
                    
                    if cache_file is not None:
                        np.savez_compressed(cache_file, train_labels=train_labels, train_weight=train_weight, train_inputs=train_inputs, features=clf.getFeatures())
                
                if plot_features:
                    self._extractor.plot(train_inputs, train_weight, train_labels, filename=plot_features_bname.replace('$i', str(i)),
                                         signal_categories=[sig_class], context=plot_context, label_2_category={ 0: bkg_class, 1: sig_class },
                                         plot_options=plot_options)
                    
                print(f'Train (count) Signal: {(train_labels == 1).sum()}; Bkg: {(train_labels == 0).sum()}')
                print(f'Train (wgted) Signal: {train_weight[train_labels == 1].sum()}; Bkg: {train_weight[train_labels == 0].sum()}')
                
                if clf.getState() == MVA_MODULE_STATES.READY and force_retrain_per_clf[i]:
                    clf = self._clfs[i] = clf.reset()

                if clf.getState() != MVA_MODULE_STATES.READY:
                    train_kwargs_current = {} if not i in train_kwargs else train_kwargs[i]
                    
                    clf.train(train_inputs, train_labels, train_weight, **train_kwargs_current)
                    clf.to_file()
                    print(clf._model.feature_importances_)
            
            if record_stats:
                print(f'Evaluating test set')
                
                cache_file = f'{cache}_test_class_{i}.npz' if cache is not None else None
                
                if cache_file is not None and isfile(cache_file):
                    cache_npz = np.load(cache_file)
                    test_labels, test_weight, test_inputs, features = cache_npz['test_labels'], cache_npz['test_weight'], cache_npz['test_inputs'], cache_npz['features']
                    features = list(features)
                    
                    self._extractor._features = features
                    self._extractor._labels = np.array([ 0, 1 ], dtype='B')
                    assert(clf.getFeatures() == features)
                else:
                    # split=1 use second split
                    test_src_idx, test_event_num, \
                    test_labels, test_weight, test_inputs = self._extractor.extract([ (bkg_class, 0), (sig_class, 1) ], clf.getFeatures(),
                                                                                step=0, split=1, weight_prop='weights_split', MOD_WEIGHT=False)
                    
                    if cache_file is not None:
                        np.savez_compressed(cache_file, test_labels=test_labels, test_weight=test_weight, test_inputs=test_inputs, features=clf.getFeatures())
                
                mva_output = clf.predict(test_inputs)[:, 1]
                
                statistics = np.zeros(len(self._threshold_scan), dtype=[('nsig', 'f'), ('nbkg', 'f'), ('threshold', 'f'), ('efficiency', 'f'), ('purity', 'f'), ('significance', 'f')])
                ntot_sig = test_weight[(test_labels == 1)].sum()
                
                for j, thresh in enumerate(self._threshold_scan):
                    statistics['nsig'][j] = test_weight[(test_labels == 1) & (mva_output >= thresh)].sum()
                    #statistics['nbkg'][j] = test_weight[(test_labels == 0) & (mva_output >= thresh)].sum()
                    statistics['nbkg'][j] = test_weight[(test_labels != 1) & (mva_output >= thresh)].sum()

                statistics['efficiency'] = statistics['nsig'] / ntot_sig
                statistics['purity'] = statistics['nsig'] / (statistics['nsig'] + statistics['nbkg'])
                statistics['significance'] = statistics['nsig'] / np.sqrt(statistics['nsig'] + statistics['nbkg'])
                statistics['threshold'] = self._threshold_scan
                
                self._stats[f'{i}.test'] = statistics
                self._optimal_cuts[i] = self._threshold_scan[np.nanargmax(statistics["significance"])]
                
    def plot_statistics(self, fname:str='mva_significances.pdf'):
        from phc import export_figures
        from zhh.plot.ild_style import legend_kwargs_fn, fig_ild_style
        import matplotlib.pyplot as plt
        
        classes = self.getClasses()
        stats_data = self.getStatistics()
        
        assert(len(stats_data))

        figs = []

        for idx, prop in enumerate(stats_data):
            stats = stats_data[f'{idx}.test']
            
            for zoomed_in, xlim in [
                (False, (0., 1.)),
                (True, (0.9, 1.))
            ]:
                fig, ax1 = plt.subplots()
                ax1.set_ylabel('Efficiency, Purity')
                ax1.set_xlim(xlim)
                
                ax2 = ax1.twinx()
                ax2.set_ylabel('Significance')
                
                best_sig = np.nanmax(stats['significance'])
                best_sig_at = self._threshold_scan[np.nanargmax(stats['significance'])]

                lns1 = ax1.plot(self._threshold_scan, stats['efficiency'], label='Efficiency')
                lns2 = ax1.plot(self._threshold_scan, stats['purity'], label='Purity')
                
                lns3 = ax2.plot(self._threshold_scan, stats['significance'], label=f'Significance' + (f' [max.: {best_sig:.3f} @ {best_sig_at:.6f}]' if xlim[0] <= best_sig_at <= xlim[1] else ''), color='green')
                if xlim[0] <= best_sig_at <= xlim[1]:
                    ax1.axvline(x=best_sig_at, color='red')
                
                lns = lns1+lns2+lns3
                ax1.legend(lns, [str(l.get_label()) for l in lns], loc='lower left', **legend_kwargs_fn())
                ax1.grid()
                
                fig_ild_style(fig, title=f'BDTG {idx+1} ({classes[0][0]} vs {classes[idx+1][0]})', xlabel='Cut Value', xunit=None,
                                yunit=None, ylabel_prefix='', ild_offset_x=0.65, ild_offset_y=0.12);
                
                if zoomed_in:
                    ax1.set_xscale('function', functions=(MulticlassModel.transform_forward, MulticlassModel.transform_inverse))
                    
                    xticks = [0.9, 0.99, 0.999, 0.9999, 0.99999, 0.999999]
                    ax1.set_xticks(xticks, labels=[str(xt) for xt in xticks], minor=False, fontsize=9)
                    ax1.minorticks_off()        
                    
                figs.append(fig)
            
        export_figures(fname, figs)