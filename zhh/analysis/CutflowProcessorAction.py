from os import getcwd, path as osp
from .CutflowProcessor import CutflowProcessor
from law import LocalTarget, LocalFileSystem, LocalFileTarget, LocalDirectoryTarget
from law.util import flatten
from collections.abc import Iterable, Sequence

config = {
    'relative_filesystem': LocalFileSystem(base=getcwd()),
    'absolute_filesystem': LocalFileSystem()
}

class CutflowProcessorAction:
    # all non-complete actions with transforms_data=True will trigger
    # a rescheduling of downstream actions with transforms_data=True
    transforms_data = True

    def __init__(self, cp:CutflowProcessor, steer:dict):
        self._cp = cp
        self._steer = steer
    
    def run(self):
        raise Exception('NotImplemented')
    
    def complete(self)->bool:
        raise Exception('NotImplemented')
    
    def reset(self):
        raise Exception('NotImplemented')

class FileBasedProcessorAction(CutflowProcessorAction):
    def __init__(self, cp: CutflowProcessor, steer:dict):
        super().__init__(cp, steer)

    def output(self)->LocalTarget|Sequence[LocalTarget]:
        raise Exception('NotImplemented')

    def complete(self)->bool:
        targets = self.output()
        if not isinstance(targets, Iterable):
            targets = [targets]
        
        for target in targets:
            if not target.exists():
                return False

        return True

    def reset(self):
        for target in flatten(self.output()):
            target.remove()
    
    @classmethod
    def localTarget(cls, path:str, is_file:bool=True, abs_path:bool|None=None):
        """Shorthand for defining a target

        Args:
            path (str): _description_
            is_file (bool, optional): _description_. Defaults to True.
            abs_path (bool, optional): _description_. Defaults to False.

        Returns:
            _type_: _description_
        """
        
        if abs_path is None:
            abs_path = osp.isabs(path)

        return (LocalFileTarget if is_file else LocalDirectoryTarget)(path,
            fs=config['absolute_filesystem'] if abs_path else config['relative_filesystem'])