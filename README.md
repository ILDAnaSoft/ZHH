# ZHH
Code for ZHH analysis with ILD

## Setup

Running the analysis requires dependencies provided by the key4hep stack *and* more. For a list of them, see [this list](#List-of-required-dependencies).

The recommended way to setup the environment variables is to use `setup.sh`. It can install all necessary dependencies via the `--install` switch (see [here](#Fresh-install)). All necessary environment variables are saved in `.env` the first time `setup.sh` runs successfully and loaded from there automatically in all subsequent runs.

If you wish to use your own local copies of the repositories, create (or edit, if it already exists) the `.env` file according to the list outlined [here](#List-of-required-dependencies).

### Fresh install

```shell
. setup.sh --install --install-dir ~/nfs/dependencies_zhh
```

This will download the necessary repositories and for ILDConfig extract LCFIPlus weights. Also, if `$CONDA_ROOT` is not found, mambaforge can be automatically downloaded and installed, setting up an python 3.11 environment called `zhh` with all dependencies defined in the `requirements.txt` file. Also, all ZHH and dependency processors will be compiled. 

After that, the framework is setup for both the analysis of individual files and the submission of batch jobs using `law` (see [here](#For-production)). 

### Using an existing setup

```shell
. setup.sh
```


### List of required dependencies

Either you or `. setup.sh --install` should make sure the following environment variables exist. If you want to specify values yourself, please use the `.env` file with the `KEY="VALUE"` syntax for compatability.

| Environment variable           | Target / Description   |
|--------------------------------|------------------------|
| REPO_ROOT                      | this repository        |
| ILD_CONFIG_DIR                 | https://github.com/iLCSoft/ILDConfig |
| MarlinML                       | https://gitlab.desy.de/ilcsoft/MarlinML |
| VariablesForDeepMLFlavorTagger | https://gitlab.desy.de/ilcsoft/variablesfordeepmlflavortagger |
| BTaggingVariables              | https://gitlab.desy.de/ilcsoft/btaggingvariables |
| DATA_PATH                      | Where all batch jobs save their outputs. Defaults to `/nfs/dust/ilc/user/$(whoami)/zhh`. |
| TORCH_PATH (*)                 | `python -c 'import torch; print(f"{torch.__file__}")'` |
| PYTHON_VERSION (**)            | Defaults to `3.11`. |

(*) autodiscovered from the key4hep-stack. Necessary for ParticleNet.   
(**) autodiscovered from the python installation of the conda environment

Optional variables are

| Environment variable         | Target / Description   |
|------------------------------|------------------------|
| SGV_DIR                      | https://gitlab.desy.de/mikael.berggren/sgv |


### Caveats

With the current key4hep stack version, SLDCorrection fails before other processors (e.g. FinalStateRecorder) can complete. This is temporarily fixed either by setting `fillRootTree=false` in the steering file or using a fixed version of SLDCorrection, see [here](https://github.com/nVentis/MarlinReco). In the latter case, create a `zhh_post_setup` function in `.env.sh` that updates `MARLIN_DLL` to point to your MarlinReco library.

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

#### Helper script to compile all ZHH processors: compile_from_scratch.sh
If you compile a freshly cloned copy of all ZHH processors from scratch, you might want to use `source compile_from_scratch.sh` from the top level directory of the repository. It saves some typing, that's all.

If you only want to integrate changes, call `source compile_from_scratch.sh keep`. This will not remove any existing build files.

## Running the analysis
For development, it is desirable to just use `Marlin` with the given 

### For testing
The analysis runs [Marlin](https://github.com/iLCSoft/Marlin) with a steering file covering the llHH, vvHH and qqHH channels at once, with individual options for jet clustering, ISR recovery + lepton pairing and hypothesis-dependent cuts.

```shell
Marlin scripts/prod.xml --constant.ILDConfigDir=$ILD_CONFIG_DIR --constant.ZHH_REPO_ROOT=$REPO_ROOT
```

For convenience, you can use the `MarlinZHH` shorthand, which automatically attaches `ILDConfigDir` as well as `ZHH_REPO_ROOT` and runs the `prod.xml` file per default. If you wish to use another file, you can use `MarlinZHH path-to-steering-file.xml`.

Supplying the environment variables is necessary because the weight files of LCFIPlus reside within `ILDConfig` and the preselection cuts are read from a JSON file inside `config`. 

### For production
For the full analysis, the framework luigi analysis workflows ([law](https://github.com/riga/law)) is used. It handles the automatic submission of batch jobs for all available physics samples. More information can be found in the README of the subdirectory `workflows`.

# Other Information

## Activating conda

### In one shell

```shell
eval "$($CONDA_ROOT/bin/conda shell.$shell_name hook)"
```

### In every terminal by default

If you want to make the conda installation by the script available from your terminal by default, run the following command (change `bash` if you're using a different shell):

```shell
cd $CONDA_ROOT/bin
./conda init bash
```

After restarting your shell, `conda activate zhh` should be available.

# Acknowledgements

Y. Radkhorrami for the correction of semi-leptonic decays. See [here](https://github.com/iLCSoft/MarlinReco/tree/master/Analysis/SLDCorrection) for the SLDCorrection processor. Also the processors [here](https://github.com/yradkhorrami/ChargedPFOCorrection) as well as [here](https://github.com/yradkhorrami/AddNeutralPFOCovMat) for necessary corrections of the covariance matrices of charged and neutral particles.