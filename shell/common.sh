#!/bin/bash

# code that is common between setup.sh and install.sh
function zhh_echo() {
    echo "ZHH> $1"
}

function zhh_recompile() {
    # Compile ZHH processors
    cd $REPO_ROOT
    unset yn
    read -p "Do you wish to keep existing binaries of compiled ZHH processors? (y) " yn
    
    if [[  "$yn" = "y" || -z "$yn" ]]; then
        source compile_from_scratch.sh keep
    else
        source compile_from_scratch.sh
    fi

    cd $REPO_ROOT
}

function zhh_git_ignore_deps() {
    cd $REPO_ROOT
    for folder in $(ls dependencies);
    do
        git rm --cached dependencies/$folder >/dev/null 2>/dev/null && echo "Removed $folder from VCS" || echo "Skipping $folder"
    done
}

if [[ -z "${MARLIN_DLL}" || $ZHH_FORCE_RELOAD -eq 1 ]]; then
    if [[ ! -f "/cvmfs/sw.hsf.org/key4hep/setup.sh" ]]; then
        zhh_echo "Error: key4hep stack not found. Make sure CVMFS is available and sw.hsf.org loaded. Aborting." && return 1
    fi
    
    if [[ ! -d "/cvmfs/sw.hsf.org/key4hep/releases/$ZHH_K4H_RELEASE" ]]; then
        zhh_echo "Warning: There is no $ZHH_K4H_RELEASE directory in /cvmfs/sw.hsf.org/key4hep/releases. Please make sure it exists (or update this script :) ). Continuing..."
    fi

    zhh_echo "Sourcing key4hep stack $ZHH_K4H_RELEASE"
    source /cvmfs/sw.hsf.org/key4hep/setup.sh -r $ZHH_K4H_RELEASE 2>&1 >/dev/null;
else
    zhh_echo "Info: key4hep stack already loaded."
fi

#########################################

if [[ ( -f "${REPO_ROOT}/.env" && -z $ZHH_ENV_DOT ) || $ZHH_FORCE_RELOAD -eq 1 ]]; then
    zhh_echo "Loading local environment file .env..."
    export $(grep -v '^#' "${REPO_ROOT}/.env" | xargs)
    export ZHH_ENV_DOT=true
fi

if [[ -f "${REPO_ROOT}/.env.sh" || $ZHH_FORCE_RELOAD -eq 1 ]]; then
    zhh_echo "Sourcing local sh file .env.sh..." 
    source "${REPO_ROOT}/.env.sh"
fi

# Use default venv name if not set
if [[ -z $ZHH_VENV_NAME ]]; then
    export ZHH_VENV_NAME="zhhvenv"
fi

# Automatically find pytorch (if not included in .env)
if [[ -z "${TORCH_PATH}" ]]; then
    zhh_echo "Trying to find pytorch..."
    export TORCH_PATH=$(dirname $(python -c 'import torch; print(f"{torch.__file__}")'))

    if [[ -d "${TORCH_PATH}" ]]; then
        zhh_echo "Found pytorch at <$TORCH_PATH>"
    else
        zhh_echo "Pytorch not found, please set TORCH_PATH. Aborting." && return 1
    fi
fi

if [[ $CMAKE_PREFIX_PATH != *"torch/share/cmake"* ]]; then
    export CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}:${TORCH_PATH}/share/cmake
    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${TORCH_PATH}/lib
fi

# Add correct gcc-runtime to LD_LIBRARY_PATH
if [[ $LD_LIBRARY_PATH != *"gcc/14.2.0-yuyjov/lib64"* ]]; then
    export LD_LIBRARY_PATH=/cvmfs/sw.hsf.org/contrib/x86_64-almalinux9-gcc11.4.1-opt/gcc/14.2.0-yuyjov/lib64:$LD_LIBRARY_PATH
fi