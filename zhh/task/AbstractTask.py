from typing import Any, Union
from collections.abc import Callable, Iterable
from uuid import uuid4

class AbstractTask():
    def __init__(self, name:str|None=None, work:Callable|None=None, args=None, kwargs:dict={}):
        if work is not None:
            self.work = work
        
        if name is None:
            name = self.__class__.__name__

        self._name = name
        self._args = args
        self._kwargs = kwargs
        self._uuid:str = str(uuid4())
        self._run_id:int = -1
        self._state = -1
        self._worker_pid = -1
        self._dependencies:dict[str, list[AbstractTask]] = {}
        self._result:Any|None = None

    def getUuid(self):
        return self._uuid

    def getRunId(self)->int:
        return self._run_id

    def setRunId(self, run_id:int):
        self._run_id = run_id
        return self

    def getName(self):
        return self._name

    def getDependencies(self):
        return self._dependencies
    
    def getResult(self):
        return self._result
    
    def setResult(self, result):
        self._result = result
        return self        

    def __eq__(self, other):
        if isinstance(other, AbstractTask):
            return self._uuid == other._uuid
        else:
            return False

    def __repr__(self):
        return f'<[{self._name}] state={self._state} id={self._uuid}>'
    
    def requires(self, group:str, dep:Union[list['AbstractTask'], 'AbstractTask']):
        """Adds a list of task dependencies under the name group. The work() method will receive the results of the
        tasks dependencies as keyword arguments: For each group, a keyword argument group:list will be supplied, with
        one entry in the list per dependent tasks. If only a single task dep is given, it will be treated as [dep]. 

        Args:
            group (str): _description_
            dep (Union[list[&#39;AbstractMPTask&#39;], &#39;AbstractMPTask&#39;]): _description_

        Raises:
            Exception: _description_
        """
        if group in self._dependencies:
            raise Exception(f'Task dependency grop <{group}> already registered')
        
        self._dependencies[group] = dep if isinstance(dep, Iterable) else [dep]

    def work(self):
        raise Exception('work() must be implemented by tasks')

    def run(self, **kwargs):
        if self._args is None:
            return self.work(**self._kwargs)
        else:
            return self.work(*self._args, **self._kwargs | kwargs)