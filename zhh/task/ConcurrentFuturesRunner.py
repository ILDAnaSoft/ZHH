from concurrent.futures import ThreadPoolExecutor, ProcessPoolExecutor, Future
from multiprocessing import cpu_count
from time import sleep
from tqdm.auto import tqdm
from queue import Empty
from typing import Any
from .AbstractTask import AbstractTask
from .AbstractRunner import AbstractRunner

class ConcurrentFuturesRunner(AbstractRunner):
    """Runner using the concurrent.futures API; can use threading and multiprocessing

        Args:
            cores (int | None, optional): _description_. Defaults to None.
            polling_freq (int | float, optional): in Hz. Defaults to 0.4.
            progress_bar (bool, optional): _description_. Defaults to True.
    """
    
    def __init__(self, mode:int, cores:int|None=None, polling_freq:int|float=2, progress_bar:bool=True):
        super().__init__()
        
        self._use_threads = mode == 1
        self._use_processes = mode == 2
        self._cores = cores if cores is not None else cpu_count()
        self._polling = 1/polling_freq
        self._progress_bar = progress_bar

    def run(self):
        if self._state != 0:
            raise Exception('MPRunner is busy')
        
        self._state = 1
        
        remaining:list[AbstractTask] = []
        started:dict[int, AbstractTask] = {}

        waiting:dict[int, tuple[list[int], AbstractTask]] = {}        
        results:dict[int, Any] = {}
        uuid_2_run_id:dict[str, int] = {}
        #run_id_2_uuid:dict[int, str] = {}

        # store running/done processes
        running:dict[int, Future] = {}
        done:dict[int, Future] = {}

        # assign run IDs (integers) to each task and
        # make sure tasks are unique
        ordered_uuids = []
        counter = 0

        # if a task has dependencies, add all of them
        def per_task(counter:int, task:AbstractTask|None=None):
            if task is None:
                task = self._tasks.pop(0)
            
            if len(task.getDependencies()):
                for group, deps in task.getDependencies().items():
                    for dep in deps:
                        counter = per_task(counter, dep)

            if task.getUuid() not in ordered_uuids:
                task.setRunId(counter)
                remaining.append(task)

                ordered_uuids.append(task.getUuid())
                uuid_2_run_id[task.getUuid()] = counter
                #run_id_2_uuid[counter] = task.getUuid()
                counter += 1

            return counter

        while len(self._tasks):
            task = self._tasks.pop(0)
            counter = per_task(counter, task)

        del ordered_uuids

        # helper fns to handle task dependencies
        def get_task_dependencies(task:AbstractTask):
            run_dependencies:list[int] = []

            for group, deps in task.getDependencies().items():
                for dep in deps:
                    run_dependencies.append(uuid_2_run_id[dep.getUuid()])

            return run_dependencies
        
        def tasks_done(run_ids:list[int], with_result:bool=False)->bool:
            for run_id in run_ids :
                if run_id not in (results if with_result else done):
                    return False

            return True

        # creates the kwargs for a task with requirements
        def collect_deps(task:AbstractTask)->dict:
            kwargs = {}
            deps = task.getDependencies()
            for group, dep_tasks in deps.items():
                kwargs[group] = []
                for dep_task in dep_tasks:
                    kwargs[group].append(results[dep_task.getRunId()])

            return kwargs

        def schedule_next(executor, next_task:AbstractTask|None=None, skip_dep_check:bool=False):
            if next_task is None and len(remaining):
                next_task = remaining.pop(0)

            if next_task is None:
                return True

            if skip_dep_check or len(next_task.getDependencies()) == 0 or tasks_done(
                get_task_dependencies(next_task), with_result=True
            ):
                started[next_task.getRunId()] = next_task

                future = executor.submit(per_work_fn, next_task, **collect_deps(next_task))
                running[next_task.getRunId()] = future

                return True
            else:
                waiting[next_task.getRunId()] = (get_task_dependencies(next_task), next_task)
                return schedule_next(executor)

        with ((ThreadPoolExecutor if self._use_threads else ProcessPoolExecutor)(max_workers=self._cores)) as executor:
            pbar = tqdm(range(len(remaining)), disable=not self._progress_bar)

            for i in range(min(self._cores, len(remaining))):
                schedule_next(executor)

            while len(remaining) > 0 or len(running) or len(waiting):
                completed = []
                #print(f'{len(running)} processes running / {len(waiting)} waiting')

                # mark processes as done
                for id, future in running.items():
                    if future.done():
                        # mark as done
                        #print(f'job {id} done')
                        done[id] = running[id]
                        completed.append(id)

                        item = future.result()
                        id, result = item
                        results[id] = result
                        started[id].setResult(result)

                # empty the queue
                #dequeue(output_queue, results)

                # schedule the next task
                # first check the waiting ones (with dependencies)
                # then other tasks            
                for id in completed:
                    del running[id]

                    pbar.update()
                    pbar.set_description(f'Finished task {started[id].getName()} with id={id}')
                    
                    # start next
                    if len(waiting):
                        # check whether any waiting task is now ready
                        sched_run_id = -1
                        #print('sched_run_id=', sched_run_id)

                        for run_id in waiting:
                            #print('waiting run_id=', run_id)
                            deps_run_ids, task = waiting[run_id]
                            #print('deps_run_ids=', deps_run_ids)

                            if tasks_done(deps_run_ids, with_result=True):
                                sched_run_id = run_id
                                break
                        
                        if sched_run_id > -1:
                            _, task = waiting[sched_run_id]
                            del waiting[sched_run_id]
                            
                            schedule_next(executor, task, True)
                        elif len(remaining):
                            schedule_next(executor)

                    elif len(remaining):
                        schedule_next(executor)

                sleep(self._polling)
                #print('n_remaining=', len(remaining), [t.getRunId() for t in remaining], 'len(running)=', len(running))

        self._state = 0

        return done

class ProcessRunner(ConcurrentFuturesRunner):
    """A task runner based on processes

    Args:
        ConcurrentFuturesRunner (_type_): _description_
    """

    def __init__(self, cores: int | None = None, polling_freq: int | float = 2, progress_bar: bool = True):
        super().__init__(2, cores, polling_freq, progress_bar)

class ThreadRunner(ConcurrentFuturesRunner):
    """A task runner based on threads

    Args:
        ConcurrentFuturesRunner (_type_): _description_
    """

    def __init__(self, cores: int | None = None, polling_freq: int | float = 2, progress_bar: bool = True):
        super().__init__(1, cores, polling_freq, progress_bar)

def per_work_fn(task:AbstractTask, **kwargs):
    #from os import getpid
    #task._worker_pid = getpid()
    for group in kwargs:
        task._kwargs[group] = kwargs[group]
        
    return ((task.getRunId(), task.run()))