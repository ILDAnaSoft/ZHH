from typing import Literal
import numpy as np

class MatrixElementProvider:
    def __init__(self, polEminus:float|int, polEplus:float|int, sqrtS:float):
        assert(-1. <= polEminus <= 1. and -1. <= polEplus <= 1.)
        assert(sqrtS > 0)
        
        self._polarization = (polEminus, polEplus)
        self._sqrtS = sqrtS

    def calcSquaredAmplitude(self, kinematics:np.ndarray, *args, **kwargs) -> np.ndarray:
        raise NotImplementedError("calcSquaredAmplitude must be implemented by inheriting class")