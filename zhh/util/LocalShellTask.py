import os, subprocess
from time import time, sleep

class LocalShellTask:
    def __init__(self, name:str, cmd:str, target:str|None=None, logfile:str|None=None):
        self._name = name
        self._cmd = cmd
        self._target = target
        self._logfile = logfile
        
        self._proc:None|subprocess.Popen = None
        self._t0 = -1
        self._tEnd = -1
    
    def getName(self)->str:
        return self._name
    
    def getTarget(self)->str|None:
        return self._target
    
    def getProcess(self)->subprocess.Popen:
        if self._proc is None:
            raise Exception(f'Task {self._name} has not been started yet.')
        
        return self._proc
    
    def start(self)->'LocalShellTask':
        if not self._proc:
            self._t0 = timestampms()
            
            popen_kwargs:dict = { 'shell': True }
            if self._logfile is not None:
                if os.path.isfile(self._logfile):
                    os.remove(self._logfile)
                
                f = open(self._logfile, 'a')
                
                popen_kwargs['stdout'] = f
                popen_kwargs['stderr'] = f
            
            self._proc = subprocess.Popen(self._cmd, **popen_kwargs)
        else:
            print(f'Task {self._name} is already running.')
            
        return self
    
    def finalize(self):
        self._tEnd = timestampms()
        dt = self._tEnd - self._t0
        
        msg = f'{self._name} took {dt/1000}s (started at {self._t0} and ended at {self._tEnd})'
        print(msg)
        
        return msg

def timestampms()->int:
    return round(time()*1000)

def execute_tasks(tasks:list[LocalShellTask], logfile:str|None='logbook.txt'):
    running:list[LocalShellTask] = []

    for task in tasks:
        target = task.getTarget()
        if target is None or not os.path.exists(target):
            print(f'Task {task.getName()} will be executed.')
            running += [task.start()]
        else:
            print(f'Task {task.getName()} already completed, skipping.')
    
    if not len(running):
        print('Nothing to do, all task targets exist.')
        return
    
    while len(running):
        for task in running:
            if task.getProcess().poll() is not None:
                msg = task.finalize()
                
                if logfile is not None:
                    with open(logfile, 'a') as f:
                        f.write(msg + '\n')
                
                running.remove(task)
                break
            
        sleep(0.05)
        
        