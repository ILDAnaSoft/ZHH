# ZHH
Code for ZHH analysis with ILD

## Setup

Running the analysis requires dependencies provided by the key4hep stack *and* more. For a list of them, see [this list](#List-of-required-dependencies).

The recommended way to setup the environment variables is to use `setup.sh`. It can install all necessary dependencies via the `--install` switch (see [here](#Fresh-install)). All necessary environment variables are saved in `.env` the first time `setup.sh` runs successfully and loaded from there automatically in all subsequent runs.

If you wish to use your own local copies of the repositories, create (or edit, if it already exists) the `.env` file according to the list outlined [here](#List-of-required-dependencies).

### Fresh install

```shell
source setup.sh --install --install-dir ~/nfs/dependencies_zhh
```

This will download the necessary repositories and for ILDConfig extract LCFIPlus weights. Also, if `$CONDA_PREFIX` is not found, mambaforge can be automatically downloaded and installed, setting up an python 3.11 environment called `zhh` with all dependencies defined in the `requirements.txt` file. Also, all ZHH and dependency processors will be compiled. 

After that, the framework is setup for both the analysis of individual files and the submission of batch jobs using `law` (see [here](#For-production)). 

### Using an existing setup

```shell
source setup.sh
```


### List of required dependencies

Either you or `source setup.sh --install` should make sure the following environment variables exist. If you want to specify values yourself, please use the `.env` file with the `KEY="VALUE"` syntax for compatability.

| Environment variable           | Target / Description   |
|--------------------------------|------------------------|
| ILD_CONFIG_DIR                 | https://github.com/iLCSoft/ILDConfig |
| MarlinML                       | https://gitlab.desy.de/ilcsoft/MarlinML |
| VariablesForDeepMLFlavorTagger | https://gitlab.desy.de/ilcsoft/variablesfordeepmlflavortagger |
| BTaggingVariables              | https://gitlab.desy.de/ilcsoft/btaggingvariables |
| CONDA_PREFIX                   | Path to the root of a conda installation. Defaults to `/nfs/dust/ilc/user/$(whoami)/miniforge3` |
| CONDA_ENV                      | Name of the environment to use. Defaults to `zhh`. |
| DATA_PATH                      | Where all batch jobs save their outputs. Defaults to `/nfs/dust/ilc/user/$(whoami)/zhh`. |
| TORCH_PATH (*)                 | `python -c 'import torch; print(f"{torch.__file__}")'` |
| PYTHON_VERSION (**)            | Defaults to `3.11`. |

(*) autodiscovered from the key4hep-stack. Necessary for ParticleNet.   
(**) autodiscovered from the python installation of the conda environment

### Compiling and installing processors

For each processor under `source/`, do
```shell
mkdir build
cd build
cmake -DCMAKE_CXX_STANDARD=17 ..
make install
```

The absolute path of the resulting library file `*.so` must then be added to `MARLIN_DLL` as via

```shell
$MARLIN_DLL=$MARLIN_DLL:<Path to compiled library file>
```

#### Helper script
If you compile a freshly cloned copy of all ZHH processors from scratch, you might want to use the `compile_from_scratch.sh` script.
For that you need to `source` it from the top level directory of the repository. It saves some typing, that's all.

## Running the analysis
For development, it is desirable to just use `Marlin` with the given 

### For testing
The `v2` analysis runs [Marlin](https://github.com/iLCSoft/Marlin) with a steering file covering the llHH, vvHH and qqHH channels at once, with individual options for jet clustering, ISR recovery + lepton pairing and hypothesis-dependent cuts.

```shell
Marlin scripts/ZHH_v2.xml --constant.ILDConfigDir=$ILD_CONFIG_DIR
```

### For production
We use the luigi analysis framework [law](https://github.com/riga/law) to orchestrate the execution of batch jobs and book-keeping of results etc. As of now, [this](https://github.com/riga/law/commit/673c2ac16eb8da9304a6c749e557f9c42ad4d976) commit is used which allows group submissions.

To prepare law for submission of jobs, first `source workflows/setup.sh`.

Then, to run the preselection analysis, execute

```shell
law run AnalysisFinal --poll-interval=120sec
```

This will run the tasks CreateRawIndex, AnalysisRuntime, CreateAnalysisChunks and then AnalysisFinal one after another and make use of the NAF when called at DESY. For each task, a folder of the same name will be created inside `$DATA_PATH`.

#### Task Overview

| Task name                 | Description           | Parameters with defaults |
|---------------------------|-----------------------|--------------------------|
| CreateRawIndex            | Creates an index of all readable sample files and physics processes associated to them. See ProcessIndex. | - |
| AnalysisRuntime           | Runs the Marlin analysis for each proc_pol combination over 50 events to estimate the runtime per event. | - |
| CreateAnalysisChunks      | Slices the sample files into chunks according to a desired normalization, physics sample size and duration per job. | - |
| AnalysisFinal             | Runs the Marlin analysis with the chunking as given above.  | - |
| AnalysisSummary           | Extracts data for analyzing the preselection and final selection, kinematic distributions etc.  ||

All tasks use a versioning that is by default `v1`.

## Central Bookkeeping

### Samples and Processes: CreateRawIndex

The task `CreateRawIndex` will use the `ProcessIndex` class to create the files `processes.npy` and `samples.npy` with lists of all available physics samples (per default: all `mc-opt-3` samples and the `hh` signal sample from the `mc-2020` production; see `get_raw_files()`) and all thereby encountered combinations of physics process and polarization.

As for all law tasks, the result will be saved at `$DATA_PATH/CreateRawIndex/v1`.

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
| xx_thrust          | float `f`               | Thrust value.                        |
| xx_e_vis           | float `f`               | Visible energy.                      |
| xx_pt_miss         | float `f`               | Missing transverse momentum.         |
| xx_invmass_miss    | float `f`               | Invariant mass of the missing pT     |
| xx_nisoleps        | unsigned byte `B`       | Number of isolated leptons.          |
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
    

# Other Information

### Making the conda installation available in your terminal by default

If you want to make the conda installation by the script available from your terminal by default, run the following command (change `bash` if you're using a different shell):

```shell
cd $CONDA_PREFIX/bin
./conda init bash
```

After restarting your shell, `conda activate zhh` should be available.

