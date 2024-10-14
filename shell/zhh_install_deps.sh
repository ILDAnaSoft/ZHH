#!/bin/bash

function zhh_install_deps() {
    local INSTALL_DIR="$1"

    if [[ ! -d "${REPO_ROOT}" ]]; then
        echo "REPO_ROOT does not point to a valid directory"
        return 1
    fi

    if [[ -d $INSTALL_DIR && ! -z "$( ls -A $INSTALL_DIR )" ]]; then
        echo "install-dir <$INSTALL_DIR> must be empty"
        return 1
    fi

    if [[ -f ".env" ]]; then
        read -p "You wish to install the dependencies, but an .env file which would be overwritten already exists. Do you wish to back it up to .env.bck and continue? Any existing .env.bck will be overwritten. (n)" yn
        if [[ "$yn" = "y" ]]; then
            rm -f .env.bck
            mv .env .env.bck
        else
            return 1
        fi
    fi

    mkdir -p $INSTALL_DIR && cd $INSTALL_DIR

    if [[ ! -d "MarlinML" ]]; then
        git clone --recurse-submodules https://gitlab.desy.de/ilcsoft/MarlinML
    fi

    if [[ ! -d "variablesfordeepmlflavortagger" ]]; then
        git clone https://gitlab.desy.de/ilcsoft/variablesfordeepmlflavortagger
    fi

    if [[ ! -d "btaggingvariables" ]]; then
        git clone https://gitlab.desy.de/ilcsoft/btaggingvariables
    fi

    if [[ ! -d "ILDConfig" ]]; then
        git clone https://github.com/iLCSoft/ILDConfig.git
    fi

    export MarlinML="$(pwd)/MarlinML"
    export VariablesForDeepMLFlavorTagger="$(pwd)/variablesfordeepmlflavortagger"
    export BTaggingVariables="$(pwd)/btaggingvariables"
    export ILD_CONFIG_DIR="$(pwd)/ILDConfig"

    # Unpack LCFIPlus weights
    if [[ -f "${ILD_CONFIG_DIR}/LCFIPlusConfig/lcfiweights/6q500_v04_p00_ildl5_c0_bdt.class.C" ]]; then
        echo "Skipping LCFIPlus weights (already exist)"
    else
        echo "Unpacking LCFIPlus weights..."

        cd "${ILD_CONFIG_DIR}/LCFIPlusConfig/lcfiweights"
        tar -xvzf 6q500_v04_p00_ildl5.tar.gz

        cd $INSTALL_DIR
    fi

    # Set DATA_PATH
    local default_data_dir="/nfs/dust/ilc/user/$(whoami)/zhh"

    read -p "Where do you want to store analysis results for batch processing? ($default_data_dir) " data_dir
    local data_dir=${data_dir:-$default_data_dir}

    # Save directories to .env
    # For CONDA_ROOT, CONDA_ENV and PYTHON_VERSION, see zhh_install_conda.sh
    cat >> "$REPO_ROOT/.env" <<EOF
REPO_ROOT="$REPO_ROOT"
MarlinML="$MarlinML"
VariablesForDeepMLFlavorTagger="$VariablesForDeepMLFlavorTagger"
BTaggingVariables="$BTaggingVariables"
TORCH_PATH="$TORCH_PATH"
ILD_CONFIG_DIR="$ILD_CONFIG_DIR"
CONDA_ROOT="$CONDA_ROOT"
CONDA_ENV="$CONDA_ENV"
PYTHON_VERSION="$PYTHON_VERSION"
DATA_PATH="$data_dir"

EOF
}
