import pickle, numpy as np
from collections.abc import Callable
from .MVAModule import MVAModule, MVA_MODULE_STATES

class SklearnModule(MVAModule):
    def __init__(self, *args, features:list[str], path:str, name:str='classifier', **kwargs):
        """Interface to MVAs implemented in Sklearn. Expects the constructor
        of the MVA as first argument, e.g. sklearn.ensemble.
        GradientBoostingClassifier.

        Args:
            name (str, optional): _description_. Defaults to 'classifier'.
        """
        super().__init__(features, path, f'{self.__class__.__name__}.{name}', *args, **kwargs)
        
        self.reset = lambda: SklearnModule(*args, features=features, path=path, name=name, **kwargs)
        
    def createModel(self, factory:Callable, model_kwargs:dict):
        print(model_kwargs)
        return factory(**model_kwargs)
    
    def _train(self, inputs:np.ndarray, labels:np.ndarray, weights:np.ndarray|None, **kwargs):
        assert(not self.isTrained())
        self._model.fit(inputs, labels, sample_weight=weights if weights is None else None, **kwargs)
        return True
    
    def predict(self, inputs:np.ndarray):
        assert(self.isTrained())
        return self._model.predict_proba(inputs)
    
    def to_file(self, path:str|None=None):
        assert(self.isTrained())
        pickle.dump((self._model, self._features), open(path if path is not None else self._path, 'wb'))

    @classmethod
    def _from_file(cls, path:str, name:str)->'SklearnModule':
        with open(path, 'rb') as pickle_file:
            model, features = pickle.load(pickle_file)
    
        module = SklearnModule(features=features, path=path, name=name, model=model)
        module._state = MVA_MODULE_STATES.READY
        
        return module