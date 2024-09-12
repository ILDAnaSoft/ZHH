# coding: utf-8

"""
Law example tasks to demonstrate HTCondor workflows at NAF.

In this file, some really basic tasks are defined that can be inherited by
other tasks to receive the same features. This is usually called "framework"
and only needs to be defined once per user / group / etc.
"""

import os, luigi, law, math
from law.config import Config
from typing import List

# the htcondor workflow implementation is part of a law contrib package
# so we need to explicitly load it
law.contrib.load("htcondor")

class HTCondorWorkflow(law.htcondor.HTCondorWorkflow):
    """
    Batch systems are typically very heterogeneous by design, and so is HTCondor. Law does not aim
    to "magically" adapt to all possible HTCondor setups which would certainly end in a mess.
    Therefore we have to configure the base HTCondor workflow in law.contrib.htcondor to work with
    the NAF environment. In most cases, like in this example, only a minimal amount of
    configuration is required.
    """

    max_runtime = law.DurationParameter(
        default=3.0,
        unit="h",
        significant=False,
        description="maximum runtime; default unit is hours; default: 1",
    )

    def htcondor_output_directory(self):
        # the directory where submission meta data should be stored
        return law.LocalDirectoryTarget(self.local_path())

    def htcondor_bootstrap_file(self):
        # each job can define a bootstrap file that is executed prior to the actual job
        # configure it to be shared across jobs and rendered as part of the job itself
        bootstrap_file = law.util.rel_path(__file__, "bootstrap.sh")
        return law.JobInputFile(bootstrap_file, share=True, render_job=True)

    def htcondor_job_config(self, config:Config, branch_keys:List, branch_values:List)->Config:
        # render_variables are rendered into all files sent with a job
        config.render_variables["analysis_path"] = os.getenv("ANALYSIS_PATH")
        config.render_variables["REPO_ROOT"] = os.getenv("REPO_ROOT")
        config.render_variables["DATA_PATH"] = os.getenv("DATA_PATH")

        # copy the entire environment
        config.custom_content.append(('getenv', 'true'))
        #config.custom_content.append(('request_cpus', '1'))
        config.custom_content.append(('request_memory', '16000 Mb'))
        
        # Only set for non-default value
        if self.max_runtime != 3.0:
            config.custom_content.append(("request_runtime", math.floor(self.max_runtime * 3600)))
        
        config.custom_content.append(('requirements', 'Machine =!= LastRemoteHost'))
        
        if len(branch_keys) > 4000:
            config.custom_content.append(('materialize_max_idle', 256))

        return config
