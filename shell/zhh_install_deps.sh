#!/bin/bash

function zhh_install_deps() {
    local INSTALL_DIR="$1"
    echo "Installing ZHH dependencies to $INSTALL_DIR"

    if [[ ! -d "${REPO_ROOT}" ]]; then
        echo "REPO_ROOT does not point to a valid directory"
        return 1
    fi

    if [[ -d $INSTALL_DIR && ! -z "$( ls -A $INSTALL_DIR )" ]]; then
        read -p "install-dir <$INSTALL_DIR> is not empty. Do you wish to continue with the existing contents? (y) " yn
        
        if [[ "$yn" != "" && "$yn" != "y" ]]; then
            echo "Aborting."
            return 1
        fi
    fi

    if [[ -f ".env" ]]; then
        local yn="n"
        
        read -p "You wish to install the dependencies, but an .env file which would be overwritten already exists. Do you wish to back it up to .env.bck and continue? Any existing .env.bck will be overwritten. (n) " yn
        if [[ "$yn" = "y" ]]; then
            rm -f .env.bck
            mv .env .env.bck
        else
            return 1
        fi
    fi

    if [[ -z $MarlinML || -z $VariablesForDeepMLFlavorTagger || -z $BTaggingVariables || -z $ILD_CONFIG_DIR ]]; then
        echo "At least one of the dependencies could not be found. Retrieving them..."

        local ind="$( [ -z "${ZSH_VERSION}" ] && echo "0" || echo "1" )" # ZSH arrays are 1-indexed
        local repositories=(
            https://gitlab.desy.de/ilcsoft/MarlinML
            https://gitlab.desy.de/ilcsoft/variablesfordeepmlflavortagger
            https://gitlab.desy.de/ilcsoft/btaggingvariables
            https://github.com/iLCSoft/ILDConfig.git)
        local varnames=(MarlinML VariablesForDeepMLFlavorTagger BTaggingVariables ILD_CONFIG_DIR)
        local dirnames=(MarlinML variablesfordeepmlflavortagger btaggingvariables ILDConfig)

        mkdir -p $INSTALL_DIR

        for dependency in ${varnames[*]}
        do
            # Check if the variables defined by varnames already exist
            if [[ -z ${!dependency} ]]; then
                echo "Dependency $dependency not found."

                local dirnamecur="${dirnames[$ind]}"

                if [[ ! -d "$INSTALL_DIR/$dirnamecur" ]]; then
                    echo "Cloning to $INSTALL_DIR/$dirnamecur"
                    git clone --recurse-submodules ${repositories[$ind]} "$INSTALL_DIR/$dirnamecur"
                else
                    echo "Directory $INSTALL_DIR/$dirnamecur already exists"
                fi

                echo "Setting variable $dependency to <$INSTALL_DIR/$dirnamecur>"
                export $dependency="$INSTALL_DIR/$dirnamecur"
            else
                echo "Dependency $dependency already found."
            fi

            ind=$((ind+1))
        done
    fi

    # Unpack LCFIPlus weights
    if [[ -f "${ILD_CONFIG_DIR}/LCFIPlusConfig/lcfiweights/6q500_v04_p00_ildl5_c0_bdt.class.C" ]]; then
        echo "Skipping LCFIPlus weights (already exist)"
    else
        echo "Unpacking LCFIPlus weights..."
        (
            cd "${ILD_CONFIG_DIR}/LCFIPlusConfig/lcfiweights" && tar -xvzf 6q500_v04_p00_ildl5.tar.gz
        )
    fi

    # Set DATA_PATH
    local default_data_dir="/nfs/dust/ilc/user/$(whoami)/zhh"

    local data_dir=""
    read -p "Where do you want to store analysis results for batch processing? ($default_data_dir) " data_dir
    local data_dir=${data_dir:-$default_data_dir}

    mkdir -p "$data_dir"

    # Save directories to .env
    # For CONDA_PREFIX, CONDA_ENV and PYTHON_VERSION, see zhh_install_conda.sh
    cat >> "$REPO_ROOT/.env" <<EOF
REPO_ROOT="$REPO_ROOT"
MarlinML="$MarlinML"
VariablesForDeepMLFlavorTagger="$VariablesForDeepMLFlavorTagger"
BTaggingVariables="$BTaggingVariables"
TORCH_PATH="$TORCH_PATH"
ILD_CONFIG_DIR="$ILD_CONFIG_DIR"
CONDA_PREFIX="$CONDA_PREFIX"
CONDA_ENV="$CONDA_ENV"
PYTHON_VERSION="$PYTHON_VERSION"
DATA_PATH="$data_dir"

EOF
}
