from typing import Any, Union, Generator, List, Optional, Literal
import numpy as np

class CutTypes():
    CUT_EQ = 0
    CUT_GTE = 1
    CUT_LTE = 2
    CUT_WINDOW = 3

CUT_TYPES = CutTypes()

class Cut():
    def __init__(self, quantity:str, label:str|None=None):
        self.quantity = quantity
        self.label = quantity if label is None else label
        self.type:int|None = None
        
    def __call__(self, arg):
        raise NotImplementedError('Method __call__ not implemented')
    
    def formula(self, unit:str|None=None):
        raise NotImplementedError('Method formula not implemented')
    
    def __repr__(self):
        return f"<Cut on {self.formula()}>"

class SemiInvisibleCut(Cut):
    def __init__(self, visible_cut:Cut, hidden_cut:Cut,
                method: Literal['AND', 'OR']='AND'):
        
        assert(method in ['AND', 'OR'])
        
        self.visible = visible_cut
        self.hidden = hidden_cut
        self.use_and = method == 'AND'
        self.use_or = method == 'OR'
        
    def __repr__(self):
        return f'<SemiInvisibleCut [Visible:{self.visible.formula()}] {"AND" if self.use_and else "OR"} [Invisible:{self.hidden.formula()}]>'
    
    def formula(self, show_hidden:bool=False):
        return self.visible.formula() + (f' {"&" if self.use_and else "|"} {self.hidden.formula()}' if show_hidden else '')
    
    def __call__(self, arg):
        if self.use_and:
            return ((self.visible(arg)) & (self.hidden(arg)))
        else:
            return ((self.visible(arg)) | (self.hidden(arg))) 
    
class ValueCut(Cut):
    def __init__(self, quantity:str, label:str|None=None):
        self.quantity = quantity
        self.label = quantity if label is None else label
        self.type:int|None = None
        
    def raw(self, arg):
        raise NotImplementedError('Method raw not implemented')

class EqualCut(ValueCut):
    def __init__(self, quantity:str, value:int, **cut_kwargs):
        super().__init__(quantity, **cut_kwargs)
        
        self.value = value
        self.type = CutTypes.CUT_EQ
    
    def __call__(self, arg):
        return self.raw(arg[self.quantity])
    
    def raw(self, arg):
        return arg == self.value
    
    def formula(self, unit:Optional[str]=None):
        return f"{self.label}{'' if unit is None else ('/' + unit)} = {self.value}"
        
class WindowCut(ValueCut):
    def __init__(self, quantity:str,
                 val1:int|float, val2:int|float,
                 center:bool=False, **cut_kwargs):
        super().__init__(quantity, **cut_kwargs)
        
        if center:
            self.lower = val1 - val2 / 2
            self.upper = val1 + val2 / 2
        else:        
            self.lower = min(val1, val2)
            self.upper = max(val1, val2)
            
        assert(self.upper > self.lower)
            
        self.type = CutTypes.CUT_WINDOW
    
    def __call__(self, arg):
        return self.raw(arg[self.quantity])
    
    def raw(self, arg):
        return (self.lower <= arg) & (arg <= self.upper)
    
    def formula(self, unit:Optional[str]=None):
        return f"{self.lower} <= {self.label}{'' if unit is None else ('/' + unit)} <= {self.upper}"

class GreaterThanEqualCut(ValueCut):
    def __init__(self, quantity:str, lower:Union[int,float], **cut_kwargs):
        super().__init__(quantity, **cut_kwargs)
        
        self.lower = lower
        self.type = CutTypes.CUT_GTE
        
    def __call__(self, arg):
        return self.raw(arg[self.quantity])

    def raw(self, arg):
        return self.lower <= arg
    
    def formula(self, unit:Optional[str]=None):
        return f"{self.label}{'' if unit is None else ('/' + unit)} >= {self.lower}"
        
class LessThanEqualCut(ValueCut):
    def __init__(self, quantity:str, upper:Union[int,float], **cut_kwargs):
        super().__init__(quantity, **cut_kwargs)
        
        self.upper = upper
        self.type = CutTypes.CUT_LTE
        
    def __call__(self, arg):
        return self.raw(arg[self.quantity])
    
    def raw(self, arg):
        return arg <= self.upper
    
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