from typing import Any, Union, Generator, List, Optional
import numpy as np

class CutTypes():
    CUT_EQ = 0
    CUT_GTE = 1
    CUT_LTE = 2
    CUT_WINDOW = 3

class Cut():
    def __init__(self, quantity:str, label:Optional[str]=None):
        self.quantity = quantity
        self.label = quantity if label is None else label
        
    def __call__(self, arg):
        raise NotImplementedError('Method __call__ not implemented')
    
    def formula(self):
        raise NotImplementedError('Method formula not implemented')
    
    def __repr__(self):
        return f"<Cut on {self.formula()}>"
    
class EqualCut(Cut):
    def __init__(self, quantity:str, value:int, **cut_kwargs):
        super().__init__(quantity, **cut_kwargs)
        
        self.value = value
        self.type = CutTypes.CUT_EQ
    
    def __call__(self, arg):
        return arg[self.quantity] == self.value
    
    def formula(self, unit:Optional[str]=None):
        return f"{self.label}{'' if unit is None else ('/' + unit)} = {self.value}"
        
class WindowCut(Cut):
    def __init__(self, quantity:str,
                 val1:Union[int,float], val2:Union[int,float],
                 center:bool=False, **cut_kwargs):
        super().__init__(quantity, **cut_kwargs)
        
        if center:
            self.lower = val1 - val2/2
            self.upper = val1 + val2/2
        else:
            self.lower = val1
            self.upper = val2
            
        self.type = CutTypes.CUT_WINDOW
    
    def __call__(self, arg):
        return (self.lower <= arg[self.quantity]) & (arg[self.quantity] <= self.upper)
    
    def formula(self, unit:Optional[str]=None):
        return f"{self.lower} <= {self.label}{'' if unit is None else ('/' + unit)} <= {self.upper}"

class GreaterThanEqualCut(Cut):
    def __init__(self, quantity:str, lower:Union[int,float], **cut_kwargs):
        super().__init__(quantity, **cut_kwargs)
        
        self.lower = lower
        self.type = CutTypes.CUT_GTE
        
    def __call__(self, arg):
        return self.lower <= arg[self.quantity]
    
    def formula(self, unit:Optional[str]=None):
        return f"{self.label}{'' if unit is None else ('/' + unit)} >= {self.lower}"
        
class LessThanEqualCut(Cut):
    def __init__(self, quantity:str, upper:Union[int,float], **cut_kwargs):
        super().__init__(quantity, **cut_kwargs)
        
        self.upper = upper
        self.type = CutTypes.CUT_LTE
        
    def __call__(self, arg):
        return arg[self.quantity] <= self.upper
    
    def formula(self, unit:Optional[str]=None):
        return f"{self.label}{'' if unit is None else ('/' + unit)} <= {self.upper}"

def apply_cuts(data:np.ndarray, cuts:List[Cut], consecutive:bool=True)->Generator:
    for i, cut in enumerate(cuts):
        if consecutive:
            if i == 0:
                a = cut(data)
            else:
                a = a & cut(data)
            
            yield a, cut
        else:
            yield cut(data), cut