from collections.abc import Callable, Mapping, Sequence
from typing import Any, Dict
import numpy as np

class LazilyLoadedObject(Sequence):
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
    """Class to lazily load properties and items. Should behave like
    a named numpy array, but only load properties when they are ac-
    cessed.
    Items are writable properties, props are read-only and must be
    set via the assignment operator and given a callback. If a given
    value is a callable, it is treated as a property, otherwise as
    an item.

    Args:
        LazilyLoadedObject (_type_): _description_
    """
    def __init__(self, length:int, mask:np.ndarray|None=None,
                 defaultHandler:Callable|None=None):
        
        """_summary_

        Args:
            length (int): _description_
            mask (np.ndarray | None, optional): _description_. Defaults to None.
            defaultHandler (Callable | None, optional): If a Callable,
                will be used at the very end of the resolution order
                to fetch a value. Defaults to None.
        """
        
        self._l0 = length
        self._length = length
        
        self._props = {}
        self._items = {}
        
        self._masks = []
        self._mask = None
        self._defaultHandler = defaultHandler
                
        if mask:
            self.maskAppend(mask)
            
    def setDefaultHandler(self, handler:Callable):
        """Sets a default handler for the container. This handler is called
        when an item is not found in the container.
        
        Args:
            handler (Callable): A callable that takes a key and returns a value.
        """
        self._defaultHandler = handler
    
    def maskAppend(self, mask:np.ndarray):
        self._masks.append(mask)
        if self._mask is None:
            self._mask = mask
        else:
            self._mask = self.masksResolve(self._masks)
            
        self._length = int(np.sum(self._mask))
        
    def masksResolve(self, masks:list[np.ndarray]):
        all = np.arange(self._l0)
        subset = np.copy(all)
        
        for mask in masks:
            subset = subset[mask]
        
        return np.isin(all, subset)
    
    def resetView(self):
        """Resets the container to its original state by clearing
        masks and setting the length to the original value.
        """
        
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
    
    def __getitem__(self, key)->np.ndarray|Any:
        if isinstance(key, np.ndarray):
            self.maskAppend(key)
            
            return self
        else: 
            res = None
            if key in self._props:
                res = self._props[key](self)
            elif key in self._items:
                res = self._items[key]
            elif self._defaultHandler is not None:
                res = self._defaultHandler(key)
            else:
                raise Exception(f'Property <{key}> not found')
            
            if res is not None:
                if self._mask is not None:
                    res = res[self._mask]
            
            return res