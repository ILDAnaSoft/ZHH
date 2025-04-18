from collections.abc import Callable
from typing import Any
import numpy as np

class LazilyLoadedObject:
    pass

class LazyTablelike(LazilyLoadedObject):
    def __init__(self, length:int):
        self._length = length
        self._props = {}
        
    def __len__(self)->int:
        return self._length
    
    def keys(self):
        return self._props.keys()
        
    def __setitem__(self, key, fetch_func:Callable[[], Any]):
        if not isinstance(fetch_func, Callable):
            raise Exception(f'fetch_func for property <{key}> must be a callable')
        
        self._props[key] = fetch_func
    
    def __getitem__(self, key):
        if key in self._props:
            return self._props[key]()
        
        raise Exception(f'Property <{key}> not found')
    
class MixedLazyTablelike(LazilyLoadedObject):
    def __init__(self, length:int, mask:np.ndarray|None=None):
        self._l0 = length
        self._length = length
        
        self._props = {}
        self._items = {}
        
        self._masks = []
        self._mask = None
        
        if mask:
            self.__maskAppend(mask)
    
    def __maskAppend(self, mask:np.ndarray):
        self._masks.append(mask)
        self._mask = self.__masksResolve(self._masks)
        self._length = int(np.sum(self._mask))
        
    def __masksResolve(self, masks:list[np.ndarray]):
        all = np.arange(self._l0)
        subset = np.copy(all)
        
        for mask in masks:
            subset = subset[mask]
        
        return np.isin(all, subset)
    
    def __masksClear(self):
        self._length = self._l0
        self._masks.clear()
        self._mask = None      
        
    def __len__(self)->int:
        return self._length
    
    def keys(self):
        return set(self._props.keys()) | set(self._items.keys())
        
    def __setitem__(self, key, any_value):
        if isinstance(any_value, Callable):
            self._props[key] = any_value
        else:
            self._items[key] = any_value     
    
    def __getitem__(self, key):
        if isinstance(key, np.ndarray):
            self.__maskAppend(key)
            
            return self
        else: 
            res = None
            if key in self._props:
                res = self._props[key]()
            elif key in self._items:
                res = self._items[key]
            else:
                raise Exception(f'Property <{key}> not found')
            
            if res is not None:
                if self._mask is not None:
                    res = res[self._mask]
            
            return res