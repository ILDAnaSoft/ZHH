from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
from ...processes.EventCategories import EventCategories
import numpy as np

class PlotObservableAction(FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, observable:str, file:str, weight_prop:str,
                 step:int|None=None, split:int|None=None, plotTop9:bool=False, signal_categories:list[str]|None=None,
                 plot_kwargs:dict={}, **kwargs):
                 
        super().__init__(cp, steer)

        self._observable = observable
        self._plot_file = file
        self._weight_prop = weight_prop

        self._step = step
        self._split = split
        self._plotTop9 = plotTop9
        self._signal_categories = signal_categories
        self._plot_kwargs = plot_kwargs
    
    def run(self):
        from phc import export_figures

        figure = self._cp.plotAt(self._observable, self._step, self._split, self._plotTop9,
                                 weight_prop=self._weight_prop, signal_category_names=self._signal_categories,
                                 plot_kwargs=self._plot_kwargs)
    
    def output(self):
        return self.localTarget(self._plot_file)