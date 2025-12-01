from .CutflowProcessor import CutflowProcessor

class CutflowProcessorAction:
    def __init__(self, cp:CutflowProcessor):
        self._cp = cp
    
    def run(self):
        raise Exception('NotImplemented')
    
    def done(self)->bool:
        raise Exception('NotImplemented')