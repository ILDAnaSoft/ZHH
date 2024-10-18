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

This will download the necessary repositories and for ILDConfig extract LCFIPlus weights. Also, if `$CONDA_ROOT` is not found, mambaforge can be automatically downloaded and installed, setting up an python 3.11 environment called `zhh` with all dependencies defined in the `requirements.txt` file. Also, all ZHH and dependency processors will be compiled. 

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
| CONDA_ROOT                     | Path to the root of a conda installation. Defaults to `/nfs/dust/ilc/user/$(whoami)/miniforge3` |
| CONDA_ENV                      | Name of the environment to use. Defaults to `zhh`. |
| DATA_PATH                      | Where all batch jobs save their outputs. Defaults to `/nfs/dust/ilc/user/$(whoami)/zhh`. |
| TORCH_PATH (*)                 | `python -c 'import torch; print(f"{torch.__file__}")'` |
| PYTHON_VERSION (**)             | Defaults to `3.11`. |

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

    $MARLIN_DLL=$MARLIN_DLL:<Path to compiled library file>

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

The `ZHH_v3_xx` steering files are different for each preselection `xx=ll,vv,qq`. 

### For production
We use the luigi analysis framework [law](https://github.com/riga/law) to orchestrate the execution of batch jobs and book-keeping of results etc. As of now, [this](https://github.com/riga/law/commit/673c2ac16eb8da9304a6c749e557f9c42ad4d976) commit is used which allows group submissions.

To prepare law for submission of jobs, first `source workflows/setup.sh`.

Then, to run the preselection analysis, execute

```shell
law run PreselectionFinal --poll-interval=120sec
```

This will run the tasks CreateRawIndex, PreselectionRuntime, CreatePreselectionChunks and then PreselectionFinal one after another and make use of the NAF when called at DESY. For each task, a folder of the same name will be created inside `$DATA_PATH`.

#### Task Overview

| Task name                 | Description           | Parameters with defaults |
|---------------------------|-----------------------|--------------------------|
| CreateRawIndex            | Creates an index of all readable sample files and physics processes associated to them. See ProcessIndex. | - |
| PreselectionRuntime       | Runs the Marlin analysis for each proc_pol combination over 50 events to estimate the runtime per event. | - |
| CreatePreselectionChunks  | Slices the sample files into chunks according to a desired normalization, physics sample size and duration per job. | - |
| PreselectionFinal         | Runs the Marlin analysis with the chunking as given above.  | - |
| PreselectionSummary       | Extracts data for analyzing the preselection and final selection, kinematic distributions etc.  ||