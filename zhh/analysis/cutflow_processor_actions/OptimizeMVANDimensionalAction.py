from typing import Literal, get_args
from json import dumps, loads
import os.path as osp

from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor, FileBasedProcessorAction
from .CutGroupProviderInterface import CutGroupProviderInterface
from .mva_tools import get_signal_categories
from tqdm.auto import tqdm
from ..DataSource import DataSource
from ..Cuts import ValueCut, GreaterThanEqualCut
import numpy as np

OptimizeMVANDimensionalMode = Literal['AGGREGATED_SUM', 'LINEAR_FIT', 'N_DIMENSIONAL_OR', 'N_DIMENSIONAL_AND']

class OptimizeMVANDimensionalAction(CutGroupProviderInterface, FileBasedProcessorAction):
    cuts:list[ValueCut] = []

    def __init__(self, cp:CutflowProcessor, steer:dict, cut_group:str, step_out:int, step:int, mva:str, split:int,
                 weight_column:str, signal_categories:list[str], cut_file:str, plot_file:str,
                 ignore_categories:list[str]=[], aggregated_signal_column:str|None=None, *kwargs):
        
        assert('mvas' in steer)

        super().__init__(cp, steer, cut_group, step_out)

        from zhh import find_by

        mva_spec = find_by(steer['mvas'], 'name', mva, is_dict=True)

        self._step = step
        self._mva_name = mva
        self._mva_classes = [ item[1] for item in mva_spec['classes'] ]
        self._mva_label = mva_spec['label_name']
        self._split = split
        self._weight_column = weight_column
        self._signal_categories = signal_categories
        self._cut_file = cut_file
        self._plot_file = plot_file

        if not len(signal_categories):
            raise Exception('At least one signal category must be defined')

        found_signal_categories = list(set(self._mva_classes) & set(self._signal_categories))
        if len(found_signal_categories) != len(self._signal_categories):
            print(f'Warning: Did not find categories {list(set(self._signal_categories) - set(found_signal_categories))} in MVA classes.' + \
                  'Optimization performance may be severed because the MVA did not take the signal category into account.')
            
        self._signal_categories_found = found_signal_categories
        self._ignore_categories = ignore_categories
        self._aggregated_signal_column = aggregated_signal_column if aggregated_signal_column is not None else self._mva_label

        assert(len(steer['signal_categories']) == len(found_signal_categories))
    
    def run(self):
        # load all MVA signal outputs and aggregated sum at the end
        score_columns = [ f'{self._mva_label}#{class_name}' for class_name in self._signal_categories_found ] + [self._aggregated_signal_column]

        masks, data, weights, _1, _2 = self._cp.snapshotAt(score_columns, self._step, self._weight_column, self._split)
        is_signal, is_background = DataSource.signal_background_masks(self._cp._sources, masks, self._signal_categories, ignore_categories=self._ignore_categories)

        print(f'Total evt count: {weights.sum():.5g}')
        print(f'Signal evt count: {weights[is_signal].sum():.5g}')
        print(f'Background evt count: {weights[is_background].sum():.5g}')

        from zhh import PlotContext, CompositeBinaryModel
        from phc import export_figures
        from scipy.optimize import minimize, differential_evolution
        import matplotlib.pyplot as plt

        # optimize thresholds
        best_significance = 0
        best_mode = ''
        best_parameters:np.ndarray

        for mode in ['AGGREGATED_SUM']:# get_args(OptimizeMVANDimensionalMode): 
            probas = list(data.values())
            objective, guess, bounds = optimization_objective_factory(probas[:-1], weights, is_signal, is_background, mode=mode)
            
            res = differential_evolution(objective, x0=guess , bounds=bounds)
            significance = 1/res.fun
            found_parameters:np.ndarray = res.x

            if significance > best_significance:
                best_significance = significance
                best_mode = mode
                best_parameters = found_parameters

        print(f'Found best significance {best_significance} with parameters {best_parameters}')

        assert(best_mode != 'LINEAR_FIT')
        # cutting on a transformation of a variable is not implemented; would need a FunctionalCut that takes in multiple columns associated with the cut
        
        self.saveCuts(mode=best_mode, parameters=np.round(best_parameters, decimals=4).tolist(),
                      columns=[ self._aggregated_signal_column ] if best_mode == 'AGGREGATED_SUM' else score_columns)
    
        # plot
        # TODO: needs revamp for mode != 'AGGREGATED_SUM'
        thresholds = CompositeBinaryModel.threshold_scan()

        discriminator = self._aggregated_signal_column if best_mode == 'AGGREGATED_SUM' else None
        if discriminator is None:
            raise NotImplementedError('best_mode != AGGREGATED_SUM')

        statistics, best_threshold = CompositeBinaryModel.doThresholdScan(data[discriminator], thresholds, weights, is_signal, is_background)
        figures = CompositeBinaryModel.plotFn(statistics, f'{self._mva_name} Signal vs Background',
                                              to_plot=[ (False, (0., 1.)) ], xlabel=self._aggregated_signal_column)

        export_figures(self.output()[1].abspath, figures)

        self.attachCutGroup()

    def saveCuts(self, **kwargs):
        with self.output()[0].open('w') as jf:
            jf.write(dumps(kwargs))
    
    def loadCuts(self):
        with self.output()[0].open('r') as jf:
            dump = loads(jf.read())
            mode = dump['mode']

        cuts = []

        if mode == 'AGGREGATED_SUM':
            cuts += [ GreaterThanEqualCut(dump['columns'][0], dump['parameters'][0]) ]
        elif mode == 'N_DIMENSIONAL_OR':
            raise Exception('WIP')

        return cuts
    
    def fetchCutGroup(self):
        if not len(self.cuts) and self.output()[0].exists():
            self.cuts = self.loadCuts()
        
        if len(self.cuts):
            return self.cuts
        else:
            raise Exception('No cuts could be found. Execute run() to find them.')

    def output(self):
        return [
            self.localTarget(self._cut_file),
            self.localTarget(self._plot_file)
        ]

    def complete(self) -> bool:
        files_exist = all([ target.exists() for target in self.output() ])
        if not files_exist:
            return False
        
        if not self._step_out in self._cp._cuts:
            print(f'Loading cuts from {self.output()[0].abspath}')

            from ..CutflowProcessor import MVAState

            self.fetchCutGroup()
            self.attachCutGroup()

        return (
            self._mva_name in self._cp._mvas and \
            self._step in self._cp._cuts
        )
    
    def reset(self):
        if self._step in self._cp._cuts:
            del self._cp._cuts[self._step]
        
        if self._mva_name in self._cp._mvas:
            del self._cp._mvas[self._mva_name]

def collect_mva_probabilities(cp:CutflowProcessor, mva_classes:list[str], step:int, split:int, label_name:str, weight_column:str, split_colum:str|None='split'):
    masks = {}
    masks_all = cp._masks[step][-1]
    tot_size = 0

    sources = cp._sources

    for source in sources:
        found = False

        for name, mask in masks_all:
            if name == source.getName():
                found = True
                break
            
        assert(found)
        masks[name] = (mask & (source.getStore()[split_colum] == split)) if split_colum is not None else mask
        tot_size += mask.sum()
    
    probas = np.zeros((tot_size, len(mva_classes)), dtype=np.float32)
    weights = np.zeros(tot_size, dtype=np.float32)

    counter = 0
    for i, source in enumerate(sources):
        src_name = source.getName()
        store = source.getStore()
        mask = masks[src_name]
        size = mask.sum()

        for j, (label, category) in enumerate(mva_classes):
            quantity = f'{label_name}#{category}'
            probas[counter:counter+size, j] = store[quantity][mask]
        
        weights[counter:counter+size] = store[weight_column][mask]

        counter += size

    return masks, probas, weights

def optimization_objective_factory(probas:list[np.ndarray], weights:np.ndarray, is_signal:np.ndarray, is_background:np.ndarray, mode:str):

    n_unknowns = 0
    bounds = [(0, 1)]
    guess = [0.5]

    use_aggregated_sum = mode == 'AGGREGATED_SUM'
    use_linear_fit = mode == 'LINEAR_FIT'
    use_and = mode == 'N_DIMENSIONAL_AND'
    use_or = mode == 'N_DIMENSIONAL_OR'

    if use_aggregated_sum:
        n_unknowns = 1
    else:
        if use_linear_fit:
            n_unknowns = len(probas)
        elif use_and or use_or:
            n_unknowns = len(probas)
        else:
            raise Exception('No mode selected. Please set either use_aggregated_sum, use_linear_fit, use_or or use_and')
        
        bounds = bounds * n_unknowns
        guess = guess * n_unknowns

    print(f'Optimizing for significance using MVA scores - Mode: <{mode}>')
    print('[Selected unweighted events] [Parameter values] [nSignal, nBackground -> Significance]')

    def objective(x, *args):
        selection = np.logical_and.reduce([ probas[i] >= x[i] for i in range(len(x)) ])

        if use_aggregated_sum:
            selection = np.sum(probas, axis=0) >= x[0]
        elif use_linear_fit:
            selection = probas[0] + ( np.sum([ [x[i] * probas[i+1]] for i in range(len(x) - 1)], axis=0) ) >= x[-1]
        elif use_and or use_or:
            # n-dimensional, and/or
            selection = (np.logical_and if use_and else np.logical_or).reduce([ probas[i] >= x[i] for i in range(len(x)) ])

        wt_sig = weights[selection & is_signal].sum()
        wt_bkg = weights[selection & is_background].sum()
        significance = wt_sig/(wt_sig + wt_bkg)**0.5

        print(f'[{selection.sum()}] [' + ', '.join([ f'{xi:.3g}' for xi in x ]) + f'] [{wt_sig:.3g}, {wt_bkg:.3g} -> {significance:.4g}]')
        return 1/significance

    return objective, guess, bounds