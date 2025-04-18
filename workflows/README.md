# ZHH Analysis with Luigi Analysis Workflows (LAW)

We use the luigi analysis framework [law](https://github.com/riga/law) to orchestrate the execution of batch jobs and book-keeping of results etc. As of now, [this](https://github.com/riga/law/commit/673c2ac16eb8da9304a6c749e557f9c42ad4d976) commit is used which allows group submissions.

## Introduction
law itself is based on the luigi framework. The terminology in both frameworks revolves around user-defined *tasks* which produce *targets* (in our case, files or directories). *Tasks* can depend on and process the results of other tasks. If all *targets* of a task exist, it is considered complete. *law* extends this in two ways: First, it allows the execution of jobs on batch farms such as NAF in a transparent manner through so-called *workflows*. Second, it introduces the concept of *branch-maps*: Dictionaries where the key defines a job number and the value the job argument (similar to queue lists from condor). In law, they can be any arbitrary structure.

An example on the NAF can be found [here](https://github.com/riga/law/tree/master/examples/htcondor_at_naf) and more information on the concepts [here](https://indico.cern.ch/event/1375573/contributions/6090022/attachments/2917075/5119365/2024-08-28_pyhepdev_law.pdf).

## Goal
For the ZHH analysis, this framework has been developed with law+luigi to minimize job times by running with the highest possible number of parallel jobs.

## Implementation and Configuration
Running an analysis requires defining and registering a configuration in `configurations.py` with a tag (name), a list of SLCIO source files and, optionally, a list of globals/constants for Marlin. Then, an index of available SLCIO files (number of events) and encountered physics processes including cross section is created. It follows a runtime analysis for a subset of each physics process sample (e.g. e2e2hh, 2f_Z_hadronic etc.) and the calculation of the chunks such that they fit the default queue of the NAF. Only after that, the jobs are finally submitted.

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

To run all tasks including `AnalysisSummary`, execute

```shell
law run AnalysisSummary --poll-interval=120sec --tag=500-all-full
```

This will run the tasks RawIndex, AnalysisRuntime, CreateAnalysisChunks and then AnalysisFinal one after another and make use of the NAF when called at DESY. Each task will create a folder of the same name under `$DATA_PATH`. The results are stored in a sub-directory that starts with the tag value.

### Task Overview

| Task name                 | Batch job | Description                                                                                                         | Parameters with defaults |
|---------------------------|-----------|---------------------------------------------------------------------------------------------------------------------|--------------------------|
| RawIndex            | No        | Creates an index of all readable sample files and physics processes associated to them. See `ProcessIndex`.          | - |
| AnalysisRuntime           | Yes       | Runs the Marlin analysis for each proc_pol (process polarization) combination over 50 events to estimate the runtime per event.            | - |
| CreateAnalysisChunks      | No        | Calculates chunks according to a desired target, physics sample size and maximum duration per job (2h, to stay below limit of 3h). `ratio` controls the number of desired events as fraction of the number of expected events (`1.` equates to ca. 45M events of the available 60M). Setting this to `None` will use all available data.     | `jobtime=7200` <br> `ratio=1.` |
| AnalysisFinal             | Yes       | Runs the Marlin analysis with the chunking as given above.                                                          | - |
| AnalysisSummary           | Yes       | Extracts data for analyzing the preselection and final selection, kinematic distributions etc.                      | - |

To use the maximum number of jobs, the chunking is done in a way such that only the normal quota is used (see [here](https://docs.desy.de/naf/documentation/job-requirements/)). 

### Task Definitions
Tasks are defined as Python classed in `tasks*.py`. Jobs which should run on HTCondor simply inherit from `HTCondorWorkflow`. Custom HTCondor settings are defined in `framework.py`. All ZHH analysis tasks which require running Marlin inherit from the `MarlinJob` class.

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

The task `AnalysisSummary` will extract data from the output of the different PreSelection processor runs and the final state recorder. Per default, this includes relevant physical distributions, b-tagging information, and truth information about the final state (which particles were produces, how many b quarks from Higgs decays). The data of many runs will be aggregated together to a number of larger chunks saved as numpy arrays.

A detailed table of the resulting data is given in the following. The quantities starting with `xx` are same for all channels, those starting with `ll`, `vv` and `vv` are different for the respective channels.

| Column             | Data type               | Description                          |
|--------------------|-------------------------|--------------------------------------|
| branch             | integer `I`             | Branch ID                            |
| pid                | unsigned short `H`      | Process ID.                          |
| event              | unsigned int `I`        | Event number.                        |
| event_category     | unsigned byte `B`       | Event category.                      |
| ll_pass            | unsigned byte `B`       | llHH pass flag (from PreSelection processor) |
| vv_pass            | unsigned byte `B`       | vvHH pass flag (-).                  |
| qq_pass            | unsigned byte `B`       | qqHH pass flag (-).                  |
| thrust          | float `f`               | Thrust value.                        |
| e_vis           | float `f`               | Visible energy.                      |
| pt_miss         | float `f`               | Missing transverse momentum.         |
| invmass_miss    | float `f`               | Invariant mass of the missing pT     |
| nisoleps        | unsigned byte `B`       | Number of isolated leptons.          |
| ll_mh1             | float `f`               | Mass of the first H boson in llHH.   |
| ll_mh2             | float `f`               | Mass of the second H boson in llHH.  |
| ll_nbjets          | unsigned byte `B`       | Number of b-jets in llHH.            |
| ll_dilepton_type   | unsigned byte `B`       | Dilepton type in llHH.               |
| ll_mz              | float `f`               | Z boson mass in llHH.                |
| ll_mz_pre_pairing  | float `f`               | Z boson mass before pairing in llHH. |
| vv_mh1             | float `f`               | See above.                           |
| vv_mh2             | float `f`               | See above.                           |
| vv_nbjets          | unsigned byte `B`       | Number of b-jets in vvHH.            |
| vv_mhh             | float `f`               | Invariant mass of the HH system in vvHH |
| qq_mh1             | float `f`               | See above.                           |
| qq_mh2             | float `f`               | See above.                           |
| qq_nbjets          | unsigned byte `B`       | Number of b-jets in qqHH.            |
| ll_bmax1           | float `f`               | Largest b-tag in llHH.               |
| ll_bmax2           | float `f`               | Second largest b-tag score in llHH.  |
| ll_bmax3           | float `f`               | Third largest b-tag score in llHH.   |
| ll_bmax4           | float `f`               | Fourth largest b-tag score in llHH.  |
| vv_bmax1           | float `f`               | See above.                           |
| vv_bmax2           | float `f`               | See above.                           |
| vv_bmax3           | float `f`               | See above.                           |
| vv_bmax4           | float `f`               | See above.                           |
| qq_bmax1           | float `f`               | See above.                           |
| qq_bmax2           | float `f`               | See above.                           |
| qq_bmax3           | float `f`               | See above.                           |
| qq_bmax4           | float `f`               | See above.                           |
    

