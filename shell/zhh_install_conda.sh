#!/bin/bash

# Script to make following environment variables available:
# - CONDA_ROOT
# - CONDA_ENV
# - PYTHON_VERSION

function zhh_install_conda() {
    local default_conda_install_dir="/nfs/dust/ilc/user/$(whoami)/miniforge3"
    local default_conda_env_name="zhh"
    local default_python_version="3.11"
    
    # Set $CONDA_ROOT
    if [[ ! -d $CONDA_ROOT ]]; then        
        if [[ ! -z $CONDA_ROOT ]]; then
            echo "Default conda install path set to <CONDA_ROOT>=<$CONDA_ROOT>"
            default_conda_install_dir=$CONDA_ROOT
        fi

        read -p "Do you with to install conda? (y) " conda_do_install
        local conda_do_install=${conda_do_install:-"y"}

        if [[ $conda_do_install != "y" ]]; then
            return 1
        fi
        
        read -p "Please enter the base path of the conda installation (nfs is recommended over afs). ($default_conda_install_dir) " conda_install_dir
        local conda_install_dir=${conda_install_dir:-$default_conda_install_dir}

        if [[ -d $conda_install_dir ]]; then
            echo "The directory <$conda_install_dir> already exists. Please remove it or choose another directory. Aborting." && return 1
        fi

        wget https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-x86_64.sh -O /tmp/miniforge.sh && bash /tmp/miniforge.sh -p $conda_install_dir || (echo "Could not install conda to <$conda_install_dir>. Aborting." && return 1 )
        export CONDA_ROOT=$conda_install_dir
    fi

    echo "<CONDA_ROOT> set to <$CONDA_ROOT>"

    # Set $CONDA_ENV and $PYTHON_VERSION
    if [[ -z $CONDA_ENV || ! -d "$CONDA_ROOT/envs/$CONDA_ENV" ]]; then
        if [[ -z $CONDA_SHLVL ]]; then
            source "${CONDA_ROOT}/etc/profile.d/conda.sh"
        fi

        if [[ ! -z $CONDA_ENV ]]; then
            echo "Default conda environment name set to <CONDA_ENV>=<$CONDA_ENV>"
            default_conda_env_name=$CONDA_ENV
        fi

        read -p "Please enter a name for the python environment. ($default_conda_env_name) " conda_env_name
        local conda_env_name=${conda_env_name:-$default_conda_env_name}

        read -p "Please enter a python version to use. ($default_python_version) " python_version
        local python_version=${python_version:-$default_python_version}

        mamba create -n $conda_env_name python=$python_version -y || (echo "Could not create conda environment <$conda_env_name> with python version <$python_version>. Aborting." && return 1)
        
        export CONDA_ENV=$conda_env_name
        export PYTHON_VERSION=$python_version
    fi

    echo "<CONDA_ENV> set to <$CONDA_ENV>"

    if [[ -z $PYTHON_VERSION ]]; then
        export PYTHON_VERSION=$("$CONDA_ROOT/envs/$CONDA_ENV/bin/python" -c 'import platform; major, minor, patch = platform.python_version_tuple(); print(f"{major}.{minor}");')
    fi

    echo "<PYTHON_VERSION> set to <$PYTHON_VERSION>"

    return 0
}
