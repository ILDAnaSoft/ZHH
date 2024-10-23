#!/bin/bash

function zhh_compile_pylcio() {
    (
    # Add pyLCIO to PYTHONPATH for the conda environment
    if [[ ! -f "$CONDA_ROOT/envs/$CONDA_ENV/lib/python$PYTHON_VERSION/site-packages/lcio.pth" ]]; then
        echo "$LCIO/python" >> "$CONDA_ROOT/envs/$CONDA_ENV/lib/python$PYTHON_VERSION/site-packages/lcio.pth"
    fi

    # Activate conda environment and build LCIO
    source "${CONDA_ROOT}/etc/profile.d/conda.sh" && conda activate $CONDA_ROOT/envs/$CONDA_ENV && cd $LCIO && rm -rf build && mkdir -p build && cd build && cmake -DBUILD_ROOTDICT=ON -D CMAKE_CXX_STANDARD=17 .. && make install

    # Add compiled LCIO to LD_LIBRARY_PATH
    if [[ $(conda env config vars list) == *"LD_LIBRARY_PATH"* ]]; then
        echo "LD_LIBRARY_PATH already set"
    else
        echo "Adding LCIO to LD_LIBRARY_PATH"
        conda env config vars set LD_LIBRARY_PATH=$LCIO/build/lib64:$LD_LIBRARY_PATH
    fi

    ) || return 1
}

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
        
        read -p "You wish to install the dependencies, but an .env file which would be overwritten already exists. Do you wish to continue anyway? (y) " yn
        if [[ "$yn" = "y" ]]; then
            rm -f .env.bck
            mv .env .env.bck
        else
            return 1
        fi
    fi

    if [[ -z $MarlinML || -z $VariablesForDeepMLFlavorTagger || -z $BTaggingVariables || -z $LCIO || -z $ILD_CONFIG_DIR ]]; then
        echo "At least one of the dependencies could not be found. Retrieving them..."

        local ind="$( [ -z "${ZSH_VERSION}" ] && echo "0" || echo "1" )" # ZSH arrays are 1-indexed
        local repositories=(
            https://gitlab.desy.de/ilcsoft/MarlinML
            https://gitlab.desy.de/ilcsoft/variablesfordeepmlflavortagger
            https://gitlab.desy.de/ilcsoft/btaggingvariables
            https://github.com/iLCSoft/LCIO.git
            https://github.com/iLCSoft/ILDConfig.git)
        local varnames=(MarlinML VariablesForDeepMLFlavorTagger BTaggingVariables LCIO ILD_CONFIG_DIR)
        local dirnames=(MarlinML variablesfordeepmlflavortagger btaggingvariables LCIO ILDConfig)

        mkdir -p $INSTALL_DIR

        for dependency in ${varnames[*]}
        do
            # Check if the variables defined by varnames already exist
            if [[ -z ${!dependency} ]]; then
                local install_dir
                local ypath="y"
                read -p "Dependency $dependency not found. You can either install it (y) or supply a path to it (enter path): " ypath

                if [[ $ypath = "y" || -z $ypath ]];
                    local dirnamecur="${dirnames[$ind]}"
                    install_dir="$INSTALL_DIR/$dirnamecur"

                    if [[ ! -d "$install_dir" ]]; then
                        echo "Cloning to $INSTALL_DIR/$dirnamecur"
                        git clone --recurse-submodules ${repositories[$ind]} "$install_dir"
                    else
                        echo "Directory $install_dir already exists. Assume it's correct."
                    fi
                elif [[ -d $ypath ]]; then
                    install_dir="$ypath"
                    echo "Using user-supplied path $ypath for dependency $dependency"
                else
                    echo "Path $ypath does not exist. Aborting..."
                    return 1
                fi

                echo "Setting variable $dependency to <$install_dir>"
                export $dependency="$install_dir"
                echo "$dependency=$install_dir" >> $REPO_ROOT/.env

                if [[ $dependency = "LCIO" ]]; then
                    echo "Compiling LCIO+pyLCIO..."
                    zhh_compile_pylcio
                fi

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
    cat > "$REPO_ROOT/.env" <<EOF
REPO_ROOT="$REPO_ROOT"
MarlinML="$MarlinML"
VariablesForDeepMLFlavorTagger="$VariablesForDeepMLFlavorTagger"
BTaggingVariables="$BTaggingVariables"
LCIO="$LCIO"
TORCH_PATH="$TORCH_PATH"
ILD_CONFIG_DIR="$ILD_CONFIG_DIR"
CONDA_PREFIX="$CONDA_PREFIX"
CONDA_ENV="$CONDA_ENV"
PYTHON_VERSION="$PYTHON_VERSION"
DATA_PATH="$data_dir"

EOF
}
