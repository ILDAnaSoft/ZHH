#!/bin/bash

function usage() {
    echo "Usage: source setup.sh [-r <key4hep-release>] [--install [--install-dir ./install]] [--compile]"
    echo "       -r <release> : setup a specific release, if not specified the latest release will be used"
    echo "       --install, -i: downloads all dependencies, attempts to compile and install them as well as all libraries inside this repository"
    echo "       --install-dir, -d: defaults to dependencies"
    echo "       --compile, -c: recompiles all dependencies. requires all paths (dependencies) to be set"
    ecgo "       --help, -h   : print this help message"
    echo "--install and --compile are mutually exclusive"
    echo ""
    echo "Additional files which may be sourced after the key4hep stack is sourced (optional, not commited to git repository):"
    echo "       .env: environment variables in key=value format"
    echo "       .env.sh: shell script for additional environment setup"
    echo ""
    echo "Dependencies"
    echo "       MarlinML: absolute path to MarlinML repository with binaries inside lib64 (see https://gitlab.desy.de/ilcsoft/MarlinML)"
    echo "       VariablesForDeepMLFlavorTagger: absolute path to repository with binaries inside lib (see https://gitlab.desy.de/ilcsoft/variablesfordeepmlflavortagger)"
    echo "       BTaggingVariables: absolute path to repository with binaries inside lib (see https://gitlab.desy.de/ilcsoft/btaggingvariables)"
    echo "       LCIO: absolute path to LCIO installation (see https://github.com/iLCSoft/LCIO)"
}

ZHH_K4H_RELEASE_DEFAULT="2024-04-12"

function zhh_echo() {
    echo "ZHH> $1"
}

function zhh_recompile() {
    # Compile ZHH processors
    cd $REPO_ROOT
    source compile_from_scratch.sh

    compile_pkg ()
    {
        cd $1
        rm -rf build
        mkdir -p build
        cd build
        cmake -DCMAKE_CXX_STANDARD=17 ..
        make install || ( cd ../.. && return 1 )
        cd ../..
    }

    # Compile the ML and helper libraries
    for module_to_compile in "$MarlinML" "$VariablesForDeepMLFlavorTagger" "$BTaggingVariables"
    do
        compile_pkg $module_to_compile && zhh_echo "+++ Successfully compiled $module_to_compile +++" || { zhh_echo "!!! Error [$?] while trying to compile $module_to_compile !!!"; cd $REPO_ROOT; return 1; }
    done

    cd $REPO_ROOT
    
}

function zhh_attach_marlin_dlls() {
    local libs=(
        "$REPO_ROOT/source/CheatedMCOverlayRemoval/lib/libCheatedMCOverlayRemoval.so"
        "$REPO_ROOT/source/AddNeutralPFOCovMat/lib/libAddNeutralPFOCovMat.so"
        "$REPO_ROOT/source/LeptonPairing/lib/libLeptonPairing.so"
        "$REPO_ROOT/source/HdecayMode/lib/libHdecayMode.so"
        "$REPO_ROOT/source/JetTaggingComparison/lib/libJetTaggingComparison.so"
        "$REPO_ROOT/source/PreSelection/lib/libPreSelection.so"
        "$REPO_ROOT/source/FinalStateRecorder/lib/libFinalStateRecorder.so"
        "$REPO_ROOT/source/ZHHKinfitProcessors/lib/libZHHKinfitProcessors.so"
        "$MarlinML/lib64/libJetTaggers.so"
        "$VariablesForDeepMLFlavorTagger/lib/libVariablesForDeepMLFlavorTagger.so"
        "$BTaggingVariables/lib/libBTaggingVariables.so"
    )

    for lib in "${libs[@]}"; do
        if [[ ! -f "$lib" ]]; then
            zhh_echo "+++ WARNING +++ Library <$(basename $lib)> not found at $lib."
            zhh_echo "    Make sure to compile it before you start Marlin. Continuing..."
        fi

        zhh_echo "Attaching library $(basename $lib)"
        export MARLIN_DLL=$MARLIN_DLL:"$lib"
    done

    # v3 requires a recent version of ReconstructedParticleParticleIDFilterProcessor.cc 
    # https://github.com/iLCSoft/MarlinReco/blob/master/Analysis/PIDTools/src/ReconstructedParticleParticleIDFilterProcessor.cc
    # As a quick fix, one may use Uli's version
    # export MARLIN_DLL="/afs/desy.de/user/u/ueinhaus/pool/MarlinReco_v01-35/lib/libMarlinReco.so.1.35.0:$MARLIN_DLL"
}

# Inferring REPO_ROOT
if [[ ! -d "$REPO_ROOT" ]]; then
    zhh_echo "Info: Trying to infer REPO_ROOT..."

    REPO_ROOT=$(realpath "${BASH_SOURCE[${#BASH_SOURCE[@]} - 1]}")
    REPO_ROOT=$(dirname "$REPO_ROOT")

    if [[ -d "$REPO_ROOT/zhh" && -d "$REPO_ROOT/source" ]]; then
        zhh_echo "Success: Found REPO_ROOT at <$REPO_ROOT>"
    else
        zhh_echo "Error: REPO_ROOT not found. Aborting."
        return 1
    fi
fi

# Parse user input
ZHH_K4H_RELEASE=$ZHH_K4H_RELEASE_DEFAULT
ZHH_COMMAND=""

for ((i=1; i<=$#; i++)); do
    eval arg=\$$i
    eval "argn=\${$((i+1))}"
    case $arg in
        --help|-h)
            usage
            return 0
            ;;
        --install)
            ZHH_COMMAND="install"
            ;;
        --install-dir|-d)
            if [[ -z "$argn" ]]; then
                zhh_echo "Error: install-dir requires a non-empty argument. Aborting." && return 1
            else
                mkdir -p "$argn" || ( zhh_echo "Error: Could not create directory <$argn>. Aborting." && return 1 )
                ZHH_INSTALL_DIR="$( realpath "$argn" )"

                if [[ $? -ne "0" ]]; then
                    zhh_echo "Error: Could not resolve dependencies directory. Aborting." && return 1
                else
                    zhh_echo "Option: Setting install-dir to default <$ZHH_INSTALL_DIR>"
                fi
                zhh_echo "Option: Setting install-dir to <$ZHH_INSTALL_DIR>" 
            fi
            ;;
        -r)
            if [[ -z "$argn" ]]; then
                zhh_echo "Error: release requires a non-empty argument. Aborting." && return 1
            else
                zhh_echo "Option: Setting release to <$argn>"
                ZHH_K4H_RELEASE="$argn"
            fi
            ;;
        --compile|-c)
            ZHH_COMMAND="compile"
            ;;
        *)
            eval "prev=\${$((i-1))}"
            if [[ "$prev" != "-r" && "$prev" != "--install-dir" && "$prev" != "-d" ]]; then
                zhh_echo "Unknown argument $arg. Aborting.\n"
                usage
                return 1
            fi
            ;;
    esac
done

if [[ -z "${MARLIN_DLL}" ]]; then
    if [[ ! -f "/cvmfs/sw.hsf.org/key4hep/setup.sh" ]]; then
        zhh_echo "Error: key4hep stack not found. Make sure CVMFS is available and sw.hsf.org loaded. Aborting." && return 1
    fi
    source /cvmfs/sw.hsf.org/key4hep/setup.sh -r $ZHH_K4H_RELEASE
else
    zhh_echo "Info: key4hep stack already loaded."
fi

#########################################

if [[ -f "${REPO_ROOT}/.env" && -z $ZHH_ENV_DOT ]]; then
    zhh_echo "Loading local environment file .env..."
    export $(grep -v '^#' "${REPO_ROOT}/.env" | xargs)
    export ZHH_ENV_DOT=true
fi

if [[ -f "${REPO_ROOT}/.env.sh" ]]; then
    zhh_echo "Sourcing local sh file .env.sh..." 
    source "${REPO_ROOT}/.env.sh"
fi

# Automatically find pytorch
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

if [[ "$ZHH_COMMAND" = "install" ]]; then
    unset zhh_install_dir

    if [[ -z "$ZHH_INSTALL_DIR" ]]; then
        ZHH_INSTALL_DIR=$( realpath "$REPO_ROOT/dependencies" )

        read -p "Where do you wish to install all the dependencies? ($ZHH_INSTALL_DIR)" zhh_install_dir
        zhh_install_dir=${$ZHH_INSTALL_DIR:-zhh_install_dir}
    else
        zhh_install_dir="$ZHH_INSTALL_DIR"
    fi

    zhh_echo "Attempting to install dependencies to <$zhh_install_dir>..."

    source $REPO_ROOT/shell/zhh_install_conda.sh
    zhh_install_conda && zhh_echo "Successfully located conda installation." || ( zhh_echo "Could not locate conda installation. Aborting."; return 1; )

    ( echo "Initializing sub-shell to install python dependencies..."
    shell_name="$( [ -z "${ZSH_VERSION}" ] && echo "bash" || echo "zsh" )"
    eval "$($CONDA_ROOT/bin/conda shell.$shell_name hook)"
    conda activate $CONDA_ENV
    pip install -r $REPO_ROOT/requirements.txt
    )

    source $REPO_ROOT/shell/zhh_install_deps.sh
    zhh_install_deps $zhh_install_dir
    
    ZHH_COMMAND="compile"
fi

if [[ ! -d "$CONDA_ROOT/envs/$CONDA_ENV" ]]; then
    zhh_echo "Warning: <$CONDA_ROOT/envs/$CONDA_ENV> does not point to a valid conda environment."
    zhh_echo "    Job submissions via law may and calls to is_root_readable/is_json_readable will fail."
    zhh_echo "    Check your conda installation."
fi

if [[ "$ZHH_COMMAND" = "compile" ]]; then
    zhh_echo "Attempting to recompile dependencies..."
    zhh_recompile

    zhh_echo "Successfully compiled all dependencies and libraries"
fi

if [[ ! -d "$MarlinML" || ! -d "$VariablesForDeepMLFlavorTagger" || ! -d "$BTaggingVariables" ||  ! -d "$LCIO" || ! -d "${ILD_CONFIG_DIR}" ]]; then
    zhh_echo "Error: MarlinML, VariablesForDeepMLFlavorTagger, BTaggingVariables, LCIO and ILD_CONFIG_DIR must be set and point to valid directories."
    zhh_echo "    Use --install to download and/or compile them here. Aborting."
    return 1
fi

if [[ ! -f "$LCIO/lib64/rootDict_rdict.pcm" ]]; then
    zhh_echo "Error: LCIO is not correctly compiled. Make sure to compile"
    zhh_echo "    it with 'cmake -DBUILD_ROOTDICT=ON'. Aborting."
    
    return 1
fi

if [[ $MARLIN_DLL != *"libFinalStateRecorder"* ]]; then
    zhh_attach_marlin_dlls
fi

alias MarlinZHH_ll="Marlin $REPO_ROOT/scripts/ZHH_v3_ll.xml --constant.ParticleNetScriptFile=\"$MarlinML/python/particlenet.pt\" --constant.ILDConfigDir=\"$ILD_CONFIG_DIR\""
alias MarlinZHH_vv="Marlin $REPO_ROOT/scripts/ZHH_v3_vv.xml --constant.ParticleNetScriptFile=\"$MarlinML/python/particlenet.pt\" --constant.ILDConfigDir=\"$ILD_CONFIG_DIR\""
alias MarlinZHH_qq="Marlin $REPO_ROOT/scripts/ZHH_v3_qq.xml --constant.ParticleNetScriptFile=\"$MarlinML/python/particlenet.pt\" --constant.ILDConfigDir=\"$ILD_CONFIG_DIR\""
alias MarlinZHH="Marlin $REPO_ROOT/scripts/ZHH_v2.xml --constant.ILDConfigDir=\"$ILD_CONFIG_DIR\""

is_root_readable() (
    source $REPO_ROOT/shell/zhh_activate_conda.sh
    zhh_activate_conda
    
    local root_tree=${2:-None}

    if [ "$root_tree" != "None" ]; then
        local root_tree="'$root_tree'"
    fi

    local res=$(python -c "from phc import root_file_readable; print(root_file_readable('${1}', ${root_tree}))")
    
    if [ "$res" = "True" ]; then
        return 0
    else
        return 1
    fi
)

is_json_readable() (
    source $REPO_ROOT/shell/zhh_activate_conda.sh
    zhh_activate_conda

    local res=$(python -c "from phc import json_file_readable; print(json_file_readable('${1}'))")
    
    if [ "$res" = "True" ]; then
        return 0
    else
        return 1
    fi
)