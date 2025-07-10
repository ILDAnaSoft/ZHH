# coding: utf-8

"""
Law example tasks to demonstrate HTCondor workflows at NAF.

In this file, some really basic tasks are defined that can be inherited by
other tasks to receive the same features. This is usually called "framework"
and only needs to be defined once per user / group / etc.
"""

import law.contrib.htcondor.workflow
import os, luigi, law, law.util, law.contrib, law.contrib.htcondor, law.job.base, math
from typing import Optional, Union, cast, TYPE_CHECKING, Any, Literal
from collections.abc import Callable
from .utils.types import SGVOptions, WhizardOption
from law import Task

if TYPE_CHECKING:
    from analysis.tasks import RawIndex
    from analysis.tasks_reco import FastSimSGV

# the htcondor workflow implementation is part of a law contrib package
# so we need to explicitly load it
law.contrib.load("htcondor")

# Keep track of all job submissions during this session of law
# This is mainly used as a fix to submit jobs with different (higher)
# requirements in case they fail the first time. Important as for some
# samples with many jets, the jobs fail due to memory issues.
session_submissions = {}

class HTCondorWorkflow(law.contrib.htcondor.HTCondorWorkflow):
    max_runtime = law.DurationParameter(
        default=3.0, # 10.0
        unit="h",
        significant=False,
        description="maximum runtime; default unit is hours; default: 1",
    )
    
    transfer_logs = luigi.BoolParameter(
        default=True,
        significant=False,
        description="transfer job logs to the output directory; default: True",
    )
    
    initial_run_with_higher_requirements = luigi.BoolParameter(
        default=False,
        significant=False,
        description="whether or not to run with increased RAM and time requirements for the first run; default: False",
    )
    
    rerun_with_higher_requirements = luigi.BoolParameter(
        default=False,
        significant=False,
        description="whether or not to run with increased RAM and time requirements if the job is failing; default: True",
    )
    higher_req_ram_mb = 16000
    higher_req_time_hours = 12
    
    def __init__(self, *args, **kwargs):
        super(HTCondorWorkflow, self).__init__(*args, **kwargs)
        self.cwd = self.htcondor_output_directory().path
        
        #b = cast(law.contrib.htcondor.workflow.HTCondorWorkflowProxy, self.workflow_proxy)
        #self.htcondor_scheduler
        #self.process_resources()
        #c = cast(law.contrib.htcondor.HTCondorJobManager, b.job_manager)
        #print(b.job_manager)
        #c.status_names
        #raise Exception('err')
        

    def htcondor_output_directory(self):
        # the directory where submission meta data should be stored
        return law.LocalDirectoryTarget(self.local_path())

    def htcondor_bootstrap_file(self):
        # each job can define a bootstrap file that is executed prior to the actual job
        # configure it to be shared across jobs and rendered as part of the job itself
        bootstrap_file = law.util.rel_path(__file__, "bootstrap.sh")
        return law.JobInputFile(bootstrap_file, share=True, render_job=True)

    def htcondor_job_config(self, config:law.job.base.BaseJobFileFactory.Config, branch_keys:list, branch_values:list):
        # render_variables are rendered into all files sent with a job
        config.render_variables["analysis_path"] = os.getenv("ANALYSIS_PATH")
        config.render_variables["REPO_ROOT"] = os.getenv("REPO_ROOT")
        config.render_variables["DATA_PATH"] = os.getenv("DATA_PATH")

        # copy the entire environment
        #config.custom_content.append(('getenv', 'true'))
        #config.custom_content.append(('request_cpus', '1'))
        
        name:Optional[str] = None
        for key, value in config.custom_content:
            if key == 'initialdir':
                name = os.path.basename(os.path.dirname(value))
        
        if self.initial_run_with_higher_requirements or (self.rerun_with_higher_requirements and name is not None and name in session_submissions):
            print(f'(Re-)Running task {name} with increased requirements')
            
            config.custom_content.append(('request_memory', f'{self.higher_req_ram_mb} Mb'))
            config.custom_content.append(("request_runtime", math.floor(self.higher_req_time_hours * 3600)))
            
            session_submissions[name] = 0 if name not in session_submissions else session_submissions[name] + 1
        else:
            # Default config: 4GB RAM and 3h of runtime
            config.custom_content.append(('request_memory', '4000 Mb'))
            if self.max_runtime:
                config.custom_content.append(('request_runtime', math.floor(cast(int|float, self.max_runtime) * 3600)))
                
            session_submissions[name] = 0
        
        config.custom_content.append(('requirements', 'Machine =!= LastRemoteHost'))
        config.custom_content.append(('max_idle', 1000))
        
        #raise Exception('asdf')

        return config

class AnalysisConfiguration:
    tag:str
    cuts:Literal['llbbbb', 'vvbbbb', 'qqbbbb']
    
    # possible entries: MarlinBaseJob
    # e.g. 'MarlinBaseJob': { 'analysis_runtime_n_files_to_process': 0, 'steering_file': 'some path.xml' }
    task_kwargs:dict[str, dict] = {}
    
    # whizard_options: should return a list of whizard option entries,
    # where one entry is for each process to generate
    whizard_options:Optional[list[WhizardOption]] = None
    
    def sgv_requires(self, sgv_task: 'FastSimSGV', requirements:dict[str, Task]):        
        if self.whizard_options is not None:
            # when using Whizard, we require fast sim
            if not isinstance(self.sgv_inputs, Callable):
                raise Exception('sgv_inputs must be defined when generating whizard events')
            
            from analysis.tasks_generator import WhizardEventGeneration            
            requirements['whizard_event_generation'] = WhizardEventGeneration.req(sgv_task)
    
    sgv_inputs:Optional[Callable[['FastSimSGV'], tuple[list[str], list[SGVOptions|None]]]] = None

    def index_requires(self, raw_index_task: 'RawIndex'):
        """If sgv_inputs is not None, we will run SGV before
        creating the ProcessIndex.
        """
        result = []
             
        if isinstance(self.sgv_inputs, Callable):
             from analysis.tasks_reco import FastSimSGV
             fast_sim_task = FastSimSGV.req(raw_index_task)
             result.append(fast_sim_task)
             
        return result
    
    """All SLCIO files that should be included in the analysis"""
    slcio_files:Optional[Union[list[str], Callable[['FastSimSGV'], list[str]]]] = None
    
    """Fration of available events that will be used for all channels
    """
    statistics:float = 1. 
    
    """If custom_statistics is a list of entries, it will be assumed as custom_statistics
    input for the get_chunk_splits function. Each entry should have the following
    shape:
        first: a number/ratio.
        second: the physics processes
        third, optional: reference; either 'expected' or 'total'. defaults to total.
        
    Example: [100, ["e1e1hh", "e2e2hh", "e3e3hh", "e1e1qqh", "e2e2qqh", "e3e3qqh",
    "n1n1hh", "n23n23hh", "n1n1qqh", "n23n23qqh",
    "qqhh", "qqqqh"], "expected"]
    """
    custom_statistics:Optional[list] = None
    
    marlin_globals:dict[str,Union[int,float,str]] = {}
    marlin_constants:dict[str,Union[int,float,str]]|Callable[[int, Any], dict[str,Union[int,float,str]]] = {}
    
    def __init__(self):
        # if not slcio files are supplied, add the outputs from SGV
        # if any other case, slcio_files must be implemented manually
        
        if self.sgv_inputs is not None and self.slcio_files is None:
            def slcio_files(raw_index_task: 'RawIndex'):        
                input_targets = raw_index_task.input()[0]['collection'].targets.values()

                return [f.path for f in input_targets]
            
            self.slcio_files = slcio_files
        

class Registry():
    definitions:dict = {}
    
    def __init__(self, cls:Callable):
        self._cls = cls
    
    def add(self, config):
        if config.tag == '':
            raise ValueError(f'Tag must be defined for configuration')
        
        if config.tag in self.definitions:
            raise ValueError(f'Configuration with tag <{config.tag}> already exists')
        
        #if not isinstance(config, self._cls):
        #    raise ValueError(f'Configuration with tag <{config.tag}> is not of type <{self._cls.__name__}>')
        
        self.definitions[config.tag] = config
        
    def get(self, tag:str):
        if not tag in self.definitions:
            raise ValueError(f'Tag <{tag}> not a known configuration. Check configurations.py')
        
        return self.definitions[tag]
    

class AnalysisConfigurationRegistry(Registry):
    def __init__(self):
        super().__init__(AnalysisConfiguration)
        
    def add(self, config:AnalysisConfiguration):
        super().add(config)
    
    def get(self, tag:str)->AnalysisConfiguration:
        return super().get(tag)

class AggregateAnalysisConfig:
    tag: str
    sub_tags:list[str]
    cuts: str
        
class AggregateAnalysisConfigurationRegistry(Registry):
    def __init__(self):
        super().__init__(AggregateAnalysisConfig)
    

# Create the registry and load the configurations
zhh_configs = AnalysisConfigurationRegistry()
aa_configs = AggregateAnalysisConfigurationRegistry()

import analysis.configurations