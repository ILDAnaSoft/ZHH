import psutil, time, pickle, subprocess, numpy as np, matplotlib.pyplot as plt

def find_child_process(Popen:subprocess.Popen, childName:str, T0:int|float=.5):
    process = psutil.Process(Popen.pid)
    pidChild = 0

    time.sleep(T0)

    for child in process.children():
        if child.name() == childName:
            pidChild = child.pid
    
    assert(pidChild)
    return pidChild

class MemoryProfiler:
    def __init__(self, proc:subprocess.Popen|None=None, pid:int|None=None, T0:int|float=0.):
        res:dict[str, list] = { 't': [] }
        self.res:dict[str, np.ndarray] = {}
        
        if proc is not None:
            if pid is None:
                pid = proc.pid

            process = psutil.Process(pid)
            items = ['rss', 'vms', 'shared', 'text', 'lib', 'data']

            for item in items:
                res[item] = []
                
            t0ms = time.time_ns() / 1e6
            while proc.poll() is None:
                tms = time.time_ns() / 1e6
                res['t'].append(T0 * 1000 + tms - t0ms)

                mem_info = process.memory_info()
                for item in items:
                    res[item].append(getattr(mem_info, item))

                time.sleep(.1)
        
            for item in res.keys():
                if item == 't':
                    self.res[item] = np.array(res[item]) / 1000
                else:
                    self.res[item] = np.array(res[item]) / 1e6
                
    def getDump(self)->dict[str, np.ndarray]:
        assert(len(self.res.keys()) > 1)
        return self.res

    def save(self, path:str):
        dump = self.getDump()

        with open(path, 'wb') as f:
            pickle.dump(dump, f)

    def plot(self, prop:str|None=None):
        if prop is None:
            fig, axes = plt.subplots(nrows=2, ncols=3, figsize=(10, 5))
            axes = axes.flatten()
        else:
            fig, ax = plt.subplots(figsize=(6, 4))
            axes = [ax]

        i = 0

        for item in self.res.keys():
            if item == 't' or (prop is not None and item != prop):
                continue

            #fig, ax = plt.subplots()
            ax = axes[i]

            ax.plot(self.res['t'], self.res[item], label=item)
            ax.set_xlabel(rf'$t$ $[s]$')
            ax.set_ylabel(rf'${item}$ $[MB]$')
            
            i += 1

        fig.tight_layout()

        return fig, axes
    
    @classmethod
    def fromDump(cls, dump:dict[str, np.ndarray]):
        instance = MemoryProfiler(None)
        instance.res = dump

        return instance        

    @classmethod
    def fromFile(cls, path:str):
        instance = MemoryProfiler()
        with open(path, 'rb') as f:
            instance.res = pickle.load(f)

        return instance