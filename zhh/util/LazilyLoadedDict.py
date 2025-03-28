from collections.abc import Callable

class LazilyLoadedDict(dict):
    def __init__(self, length:int):
        self._length = length
        self._props = {}
        
    def __len__(self)->int:
        return self._length
        
    def __setitem__(self, key, fetch_func:Callable):
        if not isinstance(fetch_func, Callable):
            raise Exception(f'fetch_func for property <{key}> must be a callable')
        
        self._props[key] = fetch_func
    
    def __getitem__(self, key):
        if key in self._props:
            return self._props[key]()
        
        raise Exception(f'Property <{key}> not found')