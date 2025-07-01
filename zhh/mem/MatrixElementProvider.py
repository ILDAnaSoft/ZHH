from typing import Literal
import numpy as np

class MatrixElementProvider:
    def __init__(self, polEminus:Literal[-1]|Literal[1], polEplus:Literal[-1]|Literal[1], sqrtS:float):
        assert(polEminus in [-1, 1] and polEplus in [-1, 1])
        assert(sqrtS > 0)
        
        self._polarization = (polEminus, polEplus)
        self._sqrtS = sqrtS

    def calcSquaredAmplitude(self, kinematics:np.ndarray):
        raise NotImplementedError("calcSquaredAmplitude must be implemented by inheriting class")