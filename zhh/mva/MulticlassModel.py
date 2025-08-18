import numpy as np
from math import log

class MulticlassModel:
    """Base class for models consisting of one or multiple MVAModules to do
    multiclass classification
    """
    def __init__(self):
        pass
    
    @staticmethod
    def threshold_scan(sections_lin:int=512, sections_asympt:int=64, rounding:int=7):
        """Returns an array of floats for a classifier threshold scan. Linear
        from 0 to 0.9 and from then on asymptotically closer to 1.

        Args:
            sections_lin (int, optional): _description_. Defaults to 512.
            sections_asympt (int, optional): _description_. Defaults to 64.
            rounding (int, optional): _description_. Defaults to 7.

        Returns:
            _type_: _description_
        """
        
        return np.round(np.concatenate([
            np.linspace(0., .9, sections_lin, endpoint=False),
            1. - 10. ** ( -np.linspace(1, 6, sections_asympt) )
        ]), rounding)
        
    @staticmethod
    def transform_forward(x, NPARTS:int=5):
        x[x <= 0] = 1e-10
        result = - np.log(1. - x)/log(10)/NPARTS - 1/NPARTS
        result[result > 1] = 1
        return result

    @staticmethod
    def transform_inverse(y, NPARTS:int=5):
        return np.min(1. - np.exp(- (y + 1/NPARTS) * NPARTS * log(10)), 1)