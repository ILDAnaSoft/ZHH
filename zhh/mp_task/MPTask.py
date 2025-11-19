import multiprocessing
from collections.abc import Callable
from uuid import uuid4

MPWorkFn = Callable[[dict, tuple|None, dict], *]

class MPTask:
    def __init__(self, work:MPWorkFn, args=None, kwargs:dict={}):
        self._work = work
        self._args = args
        self._kwargs = kwargs
        self._uuid = uuid4()
        self._state = -1
        self._worker_id = -1
        self._dependencies = {}
    
    def addDependency(self, name:str, dep:'MPTask'):
        if name in self._dependencies:
            raise Exception(f'Task dependency with name <{name}> already registered')
        
        self._dependencies[name] = dep