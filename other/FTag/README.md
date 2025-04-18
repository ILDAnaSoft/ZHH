# Requirements

The repositories [FlavourTaggingML](https://gitlab.desy.de/bryan.bliewert/FlavorTagging_ML) and [MarlinMLFlavorTagging](https://gitlab.desy.de/ilcsoft/MarlinMLFlavorTagging). If you installed the `ZHH` project using `source setup.sh --install`, all dependencies should be setup already. In this case, you only need to setup the `.env` file in this directory.

# Overview

The pipeline for training flavor tagging consists of the following steps:

1. Feature extraction.
    - The processor VariablesForDeepMLFlavorTagger inside MarlinMLFlavorTagging is used to extract jet and PFO information using AIDA to root files. 
    - The truth labels (jet flavor) is enforced from the samples, i.e. pure bbbbbb/cccccc etc. samples are used.
    - After this, we leave the Marlin/C++ world and continue with Python scripts in the `FlavourTaggingML` repo. 
2. Input preparation.
    - Includes conversion from AIDA root files to torch tensors stored in HDF5 files.
    - Corrects NaN features etc.
3. Training on GPUs.
    - submission through condor
    - interactively in the browser, e.g. through https://maxwell-jhub.desy.de
4. Inference in Marlin through `MarlinML`



# Setup

Create an `.env`file in the FTag folder with the following entries:

`TASK_ROOT` data directory, preferably on NFS
`REPO_ROOT` ZHH Repo root
`ENV_SETUP_SCRIPT`, will default to `$REPO_ROOT/setup.sh`

We use the following `.env` file:

```
REPO_ROOT=".."
ENV_SETUP_SCRIPT="$REPO_ROOT/setup.sh"
TASK_ROOT="/data/dust/user/$(whoami)/FT_ParticleNet4Tags"
```

We use the ZHH environment as it already includes the `$FlavorTagging_ML` environment variable as well as the Python virtual environment `$ZHH_VENV_NAME`. 

# 