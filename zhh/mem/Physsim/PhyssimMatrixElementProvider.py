from ..MatrixElementProvider import MatrixElementProvider
import numpy as np

def load_physsim():
    try:
        from .build.PhyssimWrapper import calc_me_zhh, calc_me_zzh
        
        return calc_me_zhh, calc_me_zzh
        
    except Exception as e:
        raise PhyssimNotLoadedException()

class PhyssimNotLoadedException(Exception):
    def __init__(self):
        super().__init__('''The PhyssimWrapper module could not be imported.
Please make sure it is compiled correctly inside zhh/mem/Physsim by calling
`mkdir build && cd build && cmake .. && make` in zhh/mem/Physsim''')

class PhyssimMatrixElementProvider(MatrixElementProvider):
    def __init__(self, *args, **kwargs):
        super().__init__(*args)
        
class PhyssimZHH(PhyssimMatrixElementProvider):
    def calcSquaredAmplitude(self, kinematics:np.ndarray, zDecayMode:int)->np.ndarray:
        calc_me_zhh = load_physsim()[0]
        return calc_me_zhh(self._polarization[0], self._polarization[1], zDecayMode, kinematics)

class PhyssimZZH(PhyssimMatrixElementProvider):
    def calcSquaredAmplitude(self, kinematics:np.ndarray, z1DecayMode:int, z2DecayMode:int)->np.ndarray:
        calc_me_zzh = load_physsim()[1]
        return calc_me_zzh(self._polarization[0], self._polarization[1], z1DecayMode, z2DecayMode, kinematics)

