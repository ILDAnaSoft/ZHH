import numpy as np

class MVAModuleStates:
    READY = 0
    UNINITIALIZED = 1

MVA_MODULE_STATES = MVAModuleStates()

class MVAModule:
    def __init__(self, features:list[str], path:str, name:str, *args, **kwargs):
        self._features = features
        self._path = path
        self._name = name
        self._state = MVA_MODULE_STATES.UNINITIALIZED
        self._model = self.createModel(*args, **kwargs) if not 'model' in kwargs else kwargs['model']
    
    def getName(self)->str:
        return self._name
    
    def getState(self)->int:
        return self._state
    
    def getModel(self):
        return self._model
    
    def getFeatures(self)->list[str]:
        return self._features
    
    def isTrained(self):
        return self._state == MVA_MODULE_STATES.READY
    
    def train(self, inputs:np.ndarray, labels:np.ndarray, weights:np.ndarray|None=None, **kwargs):
        if self._train(inputs, labels, weights, **kwargs):
            self._state = MVA_MODULE_STATES.READY
        else:
            raise Exception('Error training the model')
        
    @classmethod
    def from_file(cls, path: str, name:str='classifier'):
        return cls._from_file(path, name)
    
    # To be implemented by children
    @classmethod
    def _from_file(cls, path: str, name:str):
        raise Exception('Not implemented')
        
    def createModel(self, *args, **kwargs):
        raise Exception('Not implemented')
    
    def _train(self, inputs:np.ndarray, labels:np.ndarray, weights:np.ndarray|None=None, **kwargs):
        raise Exception('Not implemented')
    
    def predict(self, inputs:np.ndarray)->np.ndarray:
        raise Exception('Not implemented')
    
    def to_file(self, path: str):
        raise Exception('Not implemented')
    
    def reset(self)->'MVAModule':
        raise Exception('Not implemented')