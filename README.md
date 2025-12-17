# ZHH
Code for ZHH analysis with ILD

## Setup

Running the analysis requires dependencies provided by the key4hep stack *and* more. For a list of them, see [this list](#List-of-required-dependencies).

The recommended way to setup the environment variables is to `source install.sh [--auto]`. It prepares all necessary dependencies (see [here](#Fresh-install)) and creates a `setup.sh` to load the environment in the future. All necessary environment variables are saved in `.env`.

If you wish to use your own local copies of the repositories, create (or edit, if it already exists) the `.env` file according to the list outlined [here](#List-of-required-dependencies).

### Fresh install

The easiest way to setup the whole Python and C++ environment automatically is by using a key4hep stack from CVMFS. If you have `/cvmfs/sw.hsf.org` available, you may *in bash* use 

```shell
source install.sh --auto
```

where `--auto` is a switch to use default values where possible. Please do not use zsh shell for the initial setup. The script will download all necessary dependencies into a `dependencies` directory inside the working directory of the `ZHH` repo, compile them, extract LCFIPlus weights for ILDConfig, setup a Python 3.11 virtual environment (with name `zhhvenv`) and install all required Python packages defined in the `requirements.txt` file. Finally, all Marlin processors inside `source` are compiled and a copy of the fast-sim tool SGV is installed.

To test if the setup succeeded, call `MarlinZHH --global.LCIOInputFiles=<SomeLCIOFile>` where `SomeLCIOFile` points to a new (mc2020+) DST file. This will run Marlin with the default `prod.xml` steering file (scripts directory). On DESY NAF, a valid default is given for `LCIOInputFiles`, so a simple `MarlinZHH` call suffices.

After that, the framework is setup for both the analysis of individual files and the submission of batch jobs using `law` (see [here](#For-production)). As of April 2025, we use key4hep version ```2025-01-28``` and the whole fresh install takes approx. 3.3GB.

### Using an existing setup

```shell
source setup.sh
```

After the initial setup, the file `setup.sh` file will be created, which can be sourced to load all correct dependencies. **When you update the `ZHH` repo** or want to re-create `setup.sh`, **run `source install.sh --setup`**.

### List of required dependencies

The setup script requires various environment variables to produce a working environment. If you've used ```source install.sh --auto```, they are automatically set for you.

If you want to specify values yourself, please use the `.env` file with the `KEY="VALUE"` syntax for compatability.

| Environment variable           | Target / Description   |
|--------------------------------|------------------------|
| REPO_ROOT                      | this repository        |
| ILD_CONFIG_DIR                 | https://github.com/iLCSoft/ILDConfig |
| DATA_PATH                      | Where all batch jobs save their outputs. Defaults to `/data/dust/user/$(whoami)/zhh`. |
| TORCH_PATH (*)                 | `python -c 'import torch; print(f"{torch.__file__}")'` |
| LCFIPlus | https://github.com/nVentis/LCFIPlus (onnx branch) |
| LCFIPlusConfig | https://github.com/suehara/LCFIPlusConfig |
| FlavorTagging_ML | https://gitlab.desy.de/bryan.bliewert/FlavorTagging_ML.git |
| MarlinReco | Required as [this version](https://github.com/nVentis/MarlinReco.git) here contains fixes. |
| Physsim | https://github.com/nVentis/Physsim.git |
| SGV_DIR                      | https://gitlab.desy.de/mikael.berggren/sgv |
| ONNXRUNTIMEPATH (**)         | Required for LCFIPlus inference with ONNX |

Optional:

| Environment variable           | Target / Description                                             |
|--------------------------------|------------------------------------------------------------------|
| MarlinMLFlavorTagging          | https://gitlab.desy.de/bryan.bliewert/MarlinMLFlavorTagging.git  |

(*) autodiscovered from the key4hep-stack. Necessary for ParticleNet.
(**) hardwired for each key4hep stack release at the moment (see `zhh_install.sh`)

### Comments on custom repositories

For LCFIPlus, MarlinReco and MarlinKinfit, custom repositories are used instead of the default ones. Their respective Marlin processor paths are replaced in `setup.sh`.

In the fixed version of LCFIPlus, logging can be changed or completely deactivated more rigorously as before. Also, it includes the latest version supporting ParT inference, and includes a path to point to the ONNX runtime via the ONNXRUNTIMEPATH environment variable. 

The fixed version of MarlinReco includes multiple bugfixes to SLDCorrection.

MarlinKinfit has been extended to allow

PRs are intended to be filed once the analysis has been completed and no more issues are found.

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

## Python environment

During the initial setup, a Python environment `zhhvenv` is created. If you wish, a Jupyter kernel will be installed for it and saved in your central user directory.

## Shorthands

After sourcing `setup.sh`, some aliases/shell functions are available:

- `MarlinZHH`: executes Marlin with the `prod.xml` steering file with the ILDConfigDir variable set to the one from your environment
- `zhhvenv`: activates the installed Python environment

# Acknowledgements

Y. Radkhorrami for the correction of semi-leptonic decays. See [here](https://github.com/iLCSoft/MarlinReco/tree/master/Analysis/SLDCorrection) for the SLDCorrection processor. Also the processors [here](https://github.com/yradkhorrami/ChargedPFOCorrection) as well as [here](https://github.com/yradkhorrami/AddNeutralPFOCovMat) for necessary corrections of the covariance matrices of charged and neutral particles.