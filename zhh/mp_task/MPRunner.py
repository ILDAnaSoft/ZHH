import multiprocessing as mp

class MPRunner:
    def __init__(self, cores:int|None=None):
        self._cores = cores if cores is not None else mp.cpu_count()
        self._state = 0
        self._tasks = []

    def queueTask(self, task):
        self._tasks.append()

    def run(self):
        if self._state != 0:
            raise Exception('MPRunner is busy')
        
        pass