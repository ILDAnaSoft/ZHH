from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
from ...processes.EventCategories import EventCategories
import numpy as np

class PlotObservableAction(FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, observable:str, file:str, weight_column:str='weight',
                 before_step:int|None=None, split:int|None=None, plotTop9:bool=False, signal_categories:list[str]|None=None,
                 plot_kwargs:dict={}, step:int|None=None, **kwargs):
        """Plots the distribution of an observable per event category.

        Args:
            observable (str): column to plot
            file (str): output file (PDF)
            weight_column (str, optional): column holding the event weights. Defaults to 'weight'.
            before_step (int | None, optional): plots the events *before* the cut group with this index is applied,
                i.e. after all cut groups 0 .. before_step-1: 0 means no cuts at all, 1 the events after the
                preselection (cut group 0), 2 the events after cut group 1, etc. None uses the state after the last
                cut group that has been applied when this action is executed (i.e. depends on the position of the
                action). Defaults to None.
            split (int | None, optional): select events of this data split; ignored for before_step=0. Defaults to None.
            plotTop9 (bool, optional): plot the 9 largest background categories separately. Defaults to False.
            signal_categories (list[str] | None, optional): event categories to draw as signal. Defaults to the
                signal categories of the steering file.
            plot_kwargs (dict, optional): options passed on to the plotting function. Defaults to {}.
            step (int | None, optional): deprecated alias of before_step.
        """
                 
        super().__init__(cp, steer)

        if step is not None:
            if before_step is not None:
                raise ValueError('PlotObservable: before_step and the deprecated argument step cannot both be given')

            import warnings
            warnings.warn('PlotObservable: argument <step> is deprecated; use <before_step> (same meaning)', DeprecationWarning, stacklevel=2)
            print('Warning: PlotObservable: argument <step> is deprecated; use <before_step> (same meaning)')

            before_step = step

        self._observable = observable
        self._plot_file = file
        self._weight_column = weight_column

        self._step = before_step
        self._split = split
        self._plotTop9 = plotTop9
        self._signal_categories = signal_categories
        self._plot_kwargs = plot_kwargs
    
    def run(self):
        from phc import export_figures

        figure = self._cp.plotAt(self._observable, self._step, self._split, self._plotTop9,
                                 weight_prop=self._weight_column, signal_category_names=self._signal_categories,
                                 **self._plot_kwargs)
        
        export_figures(self._plot_file, [ figure ])
    
    def output(self):
        return self.localTarget(self._plot_file)