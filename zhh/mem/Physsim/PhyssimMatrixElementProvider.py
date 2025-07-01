from ..MatrixElementProvider import MatrixElementProvider
import numpy as np

PHYSSIM_LOADED = False

class PhyssimNotLoadedException(Exception):
    def __init__(self):
        super().__init__("""The PhyssimWrapper module could not be imported.
Please make sure it is compiled correctly inside zhh/mem/Physsim""")

class PhyssimMatrixElementProvider(MatrixElementProvider):
    def __init__(self, *args, **kwargs):
        if not PHYSSIM_LOADED:
            raise PhyssimNotLoadedException()
        
        super().__init__(*args)
        
class PhyssimZHH(PhyssimMatrixElementProvider):
    def calcSquaredAmplitude(self, kinematics:np.ndarray)->np.ndarray:
        return calc_me_zhh(self._polarization[0], self._polarization[1], kinematics)

try:
    from .build.PhyssimWrapper import calc_me_zhh
    
    PHYSSIM_LOADED = True
except Exception as e:
    print('''Python PhyssimWrapper module for matrix element calculation could not be loaded.
Please call `mkdir build && cd build && cmake .. && make` in zhh/mem/Physsim''')