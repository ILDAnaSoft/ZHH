from typing import Any, Union

class CutTypes():
    CUT_EQ = 0
    CUT_GTE = 1
    CUT_LTE = 2
    CUT_WINDOW = 3

class Cut():
    def __init__(self, quantity:str):
        self.quantity = quantity
        
    def __call__(self, arg):
        raise NotImplementedError('Method not implemented')
    
    def __repr__(self):
        return f"<Cut on '{self.quantity}'>"
    
class EqualCut(Cut):
    def __init__(self, quantity:str, value:int):
        super().__init__(quantity)
        
        self.value = value
        self.type = CutTypes.CUT_EQ
    
    def __call__(self, arg):
        return arg[self.quantity] == self.value
    
    def __repr__(self):
        return f"<Cut on '{self.quantity}'={self.value}>"
        
class WindowCut(Cut):
    def __init__(self, quantity:str,
                 val1:Union[int,float], val2:Union[int,float],
                 center:bool=False):
        super().__init__(quantity)
        
        if center:
            self.lower = val1 - val2/2
            self.upper = val1 + val2/2
        else:
            self.lower = val1
            self.upper = val2
            
        self.type = CutTypes.CUT_WINDOW
    
    def __call__(self, arg):
        return (self.lower <= arg[self.quantity]) & (arg[self.quantity] <= self.upper)
    
    def __repr__(self):
        return f"<Cut on {self.lower} <= '{self.quantity}' <= {self.upper}>"

class GreaterThanEqualCut(Cut):
    def __init__(self, quantity:str, lower:Union[int,float]):
        super().__init__(quantity)
        
        self.lower = lower
        self.type = CutTypes.CUT_GTE
        
    def __call__(self, arg):
        return self.lower <= arg[self.quantity]
    
    def __repr__(self):
        return f"<Cut on '{self.quantity}' >= {self.lower}>"
        
class LessThanEqualCut(Cut):
    def __init__(self, quantity:str, upper:Union[int,float]):
        super().__init__(quantity)
        
        self.upper = upper
        self.type = CutTypes.CUT_LTE
        
    def __call__(self, arg):
        return arg[self.quantity] <= self.upper
    
    def __repr__(self):
        return f"<Cut on '{self.quantity}' <= {self.upper}>"