#!/bin/bash

function usage() {
    echo "Usage: source setup.sh [-r <key4hep-release>] [--install [--install-dir ./install]] [--compile]"
    echo "       -r <release> : setup a specific release, if not specified the latest release will be used"
    echo "       --install, -i: downloads all dependencies, attempts to compile and install them as well as all libraries inside this repository"
    echo "       --install-dir, -d: defaults to dependencies"
    echo "       --compile, -c: recompiles all dependencies. requires all paths (dependencies) to be set"
    echo "       --help, -h: print this help message"
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
}

ZHH_K4H_RELEASE_DEFAULT="2024-11-28"

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

    compile_pkg ()
    {
        cd $1
        rm -rf build
        mkdir -p build
        cd build
        (cmake -DCMAKE_CXX_STANDARD=17 .. && make install ) || ( cd ../.. && return 1 )
        cd ../..
    }

    # Compile the ML and helper libraries
    for module_to_compile in "$MarlinML" "$VariablesForDeepMLFlavorTagger" "$BTaggingVariables" "$MarlinReco"
    do
        compile_pkg $module_to_compile && zhh_echo "+++ Successfully compiled $module_to_compile +++" || { zhh_echo "!!! Error [$?] while trying to compile $module_to_compile !!!"; cd $REPO_ROOT; return 1; }
    done

    cd $REPO_ROOT
    
}

function zhh_attach_marlin_dlls() {
    local libs=(
        "$REPO_ROOT/source/CheatedMCOverlayRemoval/lib/libCheatedMCOverlayRemoval.so"
        "$REPO_ROOT/source/AddNeutralPFOCovMat/lib/libAddNeutralPFOCovMat.so"
        "$REPO_ROOT/source/ChargedPFOCorrection/lib/libChargedPFOCorrection.so"
        "$REPO_ROOT/source/LeptonPairing/lib/libLeptonPairing.so"
        "$REPO_ROOT/source/HdecayMode/lib/libHdecayMode.so"
        "$REPO_ROOT/source/JetTaggingComparison/lib/libJetTaggingComparison.so"
        "$REPO_ROOT/source/PreSelection/lib/libPreSelection.so"
        "$REPO_ROOT/source/FinalStateRecorder/lib/libFinalStateRecorder.so"
        "$REPO_ROOT/source/ZHHKinfitProcessors/lib/libZHHKinfitProcessors.so"
        "$REPO_ROOT/source/TruthRecoComparison/lib/libTruthRecoComparison.so"
	"$REPO_ROOT/source/ZinvisibleErrorAnalysis/lib/libZinvisibleErrorAnalysis.so"
        #"$MarlinML/lib64/libJetTaggers.so"
        #"$VariablesForDeepMLFlavorTagger/lib/libVariablesForDeepMLFlavorTagger.so"
        #"$BTaggingVariables/lib/libBTaggingVariables.so"
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

    REPO_ROOT="$(realpath "${BASH_SOURCE[${#BASH_SOURCE[@]} - 1]}" )"
    export REPO_ROOT="$(dirname $REPO_ROOT)"

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
ZHH_FORCE_RELOAD=0

for ((i=1; i<=$#; i++)); do
    eval arg=\$$i
    eval "argn=\${$((i+1))}"
    case $arg in
        --help|-h)
            usage
            return 0
            ;;
        --force|-f)
            ZHH_FORCE_RELOAD=1
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

if [[ -z "${MARLIN_DLL}" || $ZHH_FORCE_RELOAD -eq 1 ]]; then
    if [[ ! -f "/cvmfs/sw.hsf.org/key4hep/setup.sh" ]]; then
        zhh_echo "Error: key4hep stack not found. Make sure CVMFS is available and sw.hsf.org loaded. Aborting." && return 1
    fi
    source /cvmfs/sw.hsf.org/key4hep/setup.sh -r $ZHH_K4H_RELEASE
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

if [[ "$ZHH_COMMAND" = "install" ]]; then
    unset zhh_install_dir

    source $REPO_ROOT/shell/zhh_install.sh

    # Python virtual environment (venv)
    zhh_install_venv

    # Dependencies
    if [[ -z "$ZHH_INSTALL_DIR" ]]; then
        ZHH_INSTALL_DIR=$( realpath "$REPO_ROOT/dependencies" )

        read -p "Where do you wish to install all the dependencies? ($ZHH_INSTALL_DIR) " zhh_install_dir
        if [[ ! -z "$zhh_install_dir" ]]; then 
            ZHH_INSTALL_DIR=$zhh_install_dir
        fi
    fi

    zhh_echo "Attempting to install dependencies to <$ZHH_INSTALL_DIR>..."
    zhh_install_deps $ZHH_INSTALL_DIR
    
    ZHH_COMMAND="compile"
fi

if [[ ! -d "$REPO_ROOT/$ZHH_VENV_NAME" || ! -f "$REPO_ROOT/$ZHH_VENV_NAME/bin/activate" ]]; then
    zhh_echo "Warning: <$ZHH_VENV_NAME> does not seem to point to a valid venv."
    zhh_echo "    Job submissions via law may fail. Consider running source setup.sh --install"
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

if [[ $MARLIN_DLL != *"libFinalStateRecorder"* ]]; then
    zhh_attach_marlin_dlls
fi

function MarlinZHH() {
    local steering_file
    if [[ -f "$1" ]]; then
        steering_file=$1
        shift
    else
        steering_file="$REPO_ROOT/scripts/prod.xml"
    fi

    Marlin $steering_file --constant.ILDConfigDir="$ILD_CONFIG_DIR" --constant.ZHH_REPO_ROOT="$REPO_ROOT" "$@"
}
function zhhvenv() {
    source $REPO_ROOT/$ZHH_VENV_NAME/bin/activate
}

# Helpful for running batch jobs
source $REPO_ROOT/shell/is_json_readable.sh
source $REPO_ROOT/shell/is_root_readable.sh

# Define a zhh_post_setup function in .env.sh to finalize the environment
# This is useful e.g. if you need to link to a custom version of MarlinReco etc 
if typeset -f zhh_post_setup > /dev/null; then
    zhh_post_setup
fi

export ZHH_SETUP=1
