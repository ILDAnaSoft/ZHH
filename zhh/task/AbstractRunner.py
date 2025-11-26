from .AbstractTask import AbstractTask

class AbstractRunner:
    def __init__(self):
        self._state = 0
        self._tasks:list[AbstractTask] = []
    
    def queueTask(self, task:AbstractTask):
        self._tasks.append(task)
        return self
    
    def queueTasks(self, tasks:list[AbstractTask]):
        [self._tasks.append(t) for t in tasks]
        return self

    def run(self):
        raise Exception('run() must be implemented')