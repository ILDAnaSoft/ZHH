# ZHH Analysis with Luigi Analysis Workflows (LAW)

We use the luigi analysis framework [law](https://github.com/riga/law) to orchestrate the execution of batch jobs and book-keeping of results etc. As of now, [this](https://github.com/riga/law/commit/673c2ac16eb8da9304a6c749e557f9c42ad4d976) commit is used which allows group submissions.

## Introduction
law itself is based on the luigi framework. The terminology in both frameworks revolves around user-defined *tasks* which produce *targets* (in our case, files or directories). *Tasks* can depend on and process the results of other tasks. If all *targets* of a task exist, it is considered complete. *law* extends this in two ways: First, it allows the execution of jobs on batch farms such as NAF in a transparent manner through so-called *workflows*. Second, it introduces the concept of *branch-maps*: Dictionaries where the key defines a job number and the value the job argument (similar to queue lists from condor). In law, they can be any arbitrary structure.

An example on the NAF can be found [here](https://github.com/riga/law/tree/master/examples/htcondor_at_naf) and more information on the concepts [here](https://indico.cern.ch/event/1375573/contributions/6090022/attachments/2917075/5119365/2024-08-28_pyhepdev_law.pdf).

## Goal
For the ZHH analysis, this framework has been developed with law+luigi to minimize job times by running with the highest possible number of parallel jobs.

## Implementation and Configuration
Running an analysis requires defining and registering a configuration in `configurations.py` with a tag (name), a list of SLCIO source files and, optionally, a list of globals/constants for Marlin.

Then, an index of available SLCIO files (number of events) and encountered physics processes including cross section is created. It follows a runtime analysis for a subset of each physics process sample (e.g. e2e2hh, 2f_Z_hadronic etc.) and the calculation of the chunks such that they fit the default queue of the NAF. Only after that, the jobs are finally submitted.

Some standard configurations are given, see e.g. the tags `500-all-full`, `550-hh-fast` and `550-hh-full`.

### Usage
The submission of jobs requires the environment to be fully set up. Specifically, a Python venv with the packages defined in `requirements.txt` must be installed, all processors compiled and all environment variables set. You can let the setup script do this by calling `source setup.sh --install`. After that, `source zhhvenv/bin/activate` loads the Python environment. Check the main README for more information.

To prepare law for submitting jobs, first `cd workflows && source setup.sh`. The first time `law` is executed and every time the task definitions change, running `law index --verbose` is necessary. After that, all implemented tasks can be executed by running `law run <task name>`.

For example, to create the index of all available physics samples using the `500-all-full` configuration, execute

```shell
law run RawIndex --poll-interval=30sec --tag=500-all-full
```

Note I: It is very advisable to run all law job submissions through a `screen -R job`. Otherwise, closing the current terminal session would terminate law as well. In that case, all running batch jobs would still continue, but no jobs will be rescheduled if there are failing ones.

Note II: In case you get an error that the local condor scheduler does not run, you might need to ssh into the same or another NAF worker node.

Note III: Per default, all batch jobs are 

To run all tasks including `AnalysisCombine`, execute

```shell
law run AnalysisGroup --poll-interval=120sec --tag=550-all-full
```

This will run the tasks RawIndex, AnalysisRuntime, CreateAnalysisChunks and then AnalysisFinal one after another and make use of the NAF when called at DESY. Each task will create a folder of the same name under `$DATA_PATH`. The results are stored in a sub-directory that starts with the tag value.

### Error Handling for Condor Jobs / Law Workflows
In case jobs fail, use `condor_q -h` or take a look at the log files for why jobs have failed. The default location of log files for some task `X` is in `$DATA_ROOT/X` as `stdall_y.txt` where y is the branch number of the job. 

If the reported HOLD_REASON is "Job runtime longer than reserved", make sure that Marlin processes respective input files correctly. If you cannot find the issue, it might just randomly happen that the runtime exceeds the reserved runtime limit. As a quick fix, you may use `condor_rm $(whoami)` and afterwards rerun the law command with the optional argument `--initial-run-with-higher-requirements=True`. This will enforce higher runtime and RAM limits (6h and 4GB per default). 

### Removing task outputs

To force restart a task `X` with configuration tag `Y`, you have two options:

1. Use the command line interface: run `law run X --tag=Y --remove-output=<recursionDepth>` with recursionDepth=1.

2. Delete the directory `$DATA_ROOT$/X/Y`

### Task Overview

| Task name                 | Batch job | Description                                                                                                         | Parameters with defaults |
|---------------------------|-----------|---------------------------------------------------------------------------------------------------------------------|--------------------------|
| RawIndex                  | No        | Creates an index of all readable sample files and physics processes associated to them. See `ProcessIndex`.         | - |
| AnalysisRuntime           | Yes       | Runs Marlin prod_reco_run.xml for each proc_pol (process polarization) combination over 50 events to estimate the runtime per event.            | - |
| CreateAnalysisChunks      | No        | Calculates chunks according to a desired target, physics sample size and maximum duration per job (2h, to stay below limit of 3h). `ratio` controls the number of desired events as fraction of the number of expected events (`1.` equates to ca. 45M events of the available 60M). Setting this to `None` will use all available data.     | `jobtime=7200` <br> `ratio=1.` |
| AnalysisFinal             | Yes       | Runs the Marlin analysis with the chunking as given above.                                                          | - |
| AnalysisCombine           | Yes       | Extracts data for analyzing the preselection and final selection, kinematic distributions etc.                      | - |

To use the maximum number of jobs, the chunking is done in a way such that only the normal quota is used (see [here](https://docs.desy.de/naf/documentation/job-requirements/)). 

### Task Definitions
Tasks are defined as Python classed in `tasks*.py`. Jobs which should run on HTCondor simply inherit from `HTCondorWorkflow`. Custom HTCondor settings are defined in `framework.py`. All ZHH analysis tasks which require running Marlin inherit from the `AbstractMarlin` class.

## Central Bookkeeping

### Samples and Processes: RawIndex
The task `RawIndex` will use the `ProcessIndex` class to create the files `processes.npy` and `samples.npy` with lists of all available physics samples (per default: all `mc-opt-3` samples and the `hh` signal sample from the `mc-2020` production; see `get_raw_files()`) and all thereby encountered combinations of physics process and polarization.

#### samples.npy

For a list of data types, please refer [here](https://numpy.org/doc/stable/reference/arrays.scalars.html#integer-types).

| Column      | Data type               | Description                                            |
|-------------|-------------------------|--------------------------------------------------------|
| sid         | integer `i`             | Sample ID and index in this array.                     |
| run_id      | integer `i`             | Run ID as saved in the LCIO.                           |
| process     | unicode string `<U60`   | From LCIO                                              |
| proc_pol    | unicode string `<U64`   | {process}_XY with (X,Y) = (e-,e+) polarization ∈ {L,R} |
| n_events    | integer `i`             | From LCIO                                              |
| pol_e       | integer `i`             | e- polarization; -1 -> L and +1 -> R                   |
| pol_p       | integer `i`             | e+ polarization, same                                  |
| location    | unicode string `<U512`  | Full absolute path to the raw slcio file               |

#### processes.npy

| Column        | Data type               | Description                          |
|---------------|-------------------------|--------------------------------------|
| pid           | integer `i`             | Process ID and index in this array.  |
| process       | unicode string `<U60`   | See above.                           |
| proc_pol      | unicode string `<U64`   | See above.                           |
| pol_e         | integer `i`             | See above.                           |
| pol_p         | integer `i`             | See above.                           |
| cross_sec     | float `f`               | Cross section [fb].                  |
| cross_sec_err | float `f`               | Cross section error [fb].            |
| generator_id  | integer `i`             | Generator ID. From LCIO.             |

### Chunking Pre-Analysis: chunks.npy

| Column       | Data type               | Description                              |
|--------------|-------------------------|------------------------------------------|
| branch       | integer `I`             | Branch ID and index in this array.       |
| process      | unicode string `<U60`   | See above.                               |
| proc_pol     | unicode string `<U64`   | See above.                               |
| location     | unicode string `<U512`  | See above.                               |
| n_chunks     | integer `I`             | Number of chunks for file at `location`. |
| chunk_start  | integer `I`             | Start index of the chunk.                |
| chunk_size   | integer `I`             | Size of the chunk.                       |

In production mode, this chunking is used as branch map for the run of the `AnalysisFinal` task.

The number of events considered by the analysis is defined in the `CreateAnalysisChunks` task.
This can be controlled via the `--ratio` argument: For `None`, all data will be used. If a float `r`
is given (default, 1.0), for any combination of process and polarization, 
`min(r*expected number of events, total number of events)` will be used.

### Chunking Post-Analysis: chunks_f.npy

Copy of `chunks.npy` with an additional column `chunk_size_factual` of integer type `I` representing the number of actually processed events. A sub-percent difference can be expected. 

### Analysis Results

The task `AnalysisCombine` will extract and combine the output from the AnalysisFinal task. This covers the TTrees EventObservablesLL/VV and FinalStateRecorder. All data of one tag will be aggregated together in one ROOT file.
    
### Implementation Specifics
The `configurations.py` includes instructions to dynamically inject task dependencies. This is important for all tasks which have the obligatory `--tag` parameter. 