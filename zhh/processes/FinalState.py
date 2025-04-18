# TODO:
# class LazilyLoadedObject

import uproot as ur
import numpy as np
from collections.abc import Callable
import abc
from .FinalStateDict import FinalStateDict

class FinalState:
    __metaclass__ = abc.ABCMeta
    
    def __init__(self, name:str, definition:Callable[[dict[str, np.ndarray]], np.ndarray]):
        self._name = name
        self._definition = definition
    
    def evaluate(self, tree:ur.TTree):
        return self._definition(FinalStateDict(tree))
    
    @abc.abstractmethod
    def calcCrossSection(self, processes:np.ndarray, tree:ur.WritableTree):
        pass
    
    @abc.abstractmethod
    def calcExpectedEvents(self, tree:ur.WritableTree):
        pass # return self._definition(tree)
        