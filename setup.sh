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
}

ZHH_K4H_RELEASE_DEFAULT="2024-04-12"

function zhh_install() {
    local INSTALL_DIR="$1"

    if [ -d $INSTALL_DIR ]; then
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

    git clone --recurse-submodules https://gitlab.desy.de/ilcsoft/MarlinML
    git clone https://gitlab.desy.de/ilcsoft/variablesfordeepmlflavortagger
    git clone https://gitlab.desy.de/ilcsoft/btaggingvariables

    export MarlinML="$(pwd)/MarlinML"
    export VariablesForDeepMLFlavorTagger="$(pwd)/variablesfordeepmlflavortagger"
    export BTaggingVariables="$(pwd)/btaggingvariables"

    # Save directories to .env
    cat >> "$REPO_ROOT/.env" <<EOF
MarlinML="$MarlinML"
VariablesForDeepMLFlavorTagger="$VariablesForDeepMLFlavorTagger"
BTaggingVariables="$BTaggingVariables"
TORCH_PATH="$TORCH_PATH"

EOF
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
        make install || { cd ../.. ; return 1; }
        cd ../..
    }

    # Compile the ML and helper libraries
    for module_to_compile in "$MarlinML" "$VariablesForDeepMLFlavorTagger" "$BTaggingVariables"
    do
        compile_pkg $module_to_compile && echo "+++ Successfully compiled $module_to_compile +++" || { echo "!!! Error [$?] while trying to compile $module_to_compile !!!"; cd $REPO_ROOT; return 1; }
    done

    cd $REPO_ROOT
    
}

function zhh_attach_marlin_dlls() {
    # Starting July 2024, libnsl.so.1 cannot be found. They seem to be not available on batch nodes only, but are present on the local machines
    # As a temporary (?) workaround, we use a clone of the lib64 directory from the WGS node (they use nearly the same version)
    # export MARLIN_DLL=$(echo "$MARLIN_DLL" | sed "s~/cvmfs/ilc.desy.de/key4hep/releases/2023-05-23/pandoraanalysis/2.0.1/x86_64-centos7-gcc12.3.0-opt/oqkyr/lib/libPandoraAnalysis.so:~~g")
    # export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/nfs/dust/ilc/user/bliewert/lib64

    # Debugging LCFIPlus
    # 2024-03-10
    #export MARLIN_DLL=$(echo $MARLIN_DLL | sed -e "s#cvmfs/sw.hsf.org/key4hep/releases/2024-03-10/x86_64-centos7-gcc12.2.0-opt/lcfiplus/0.10.1-ff6lg4#root/public/DevLocal/LCFIPlus#g")
    
    # 2023-11-23
    #export MARLIN_DLL=$(echo $MARLIN_DLL | sed -e "s#cvmfs/sw.hsf.org/key4hep/releases/2023-11-23/x86_64-centos7-gcc12.2.0-opt/lcfiplus/0.10.1-z7amkm#root/public/DevLocal/LCFIPlus#g")    

    local libs=(
        "$REPO_ROOT/source/CheatedMCOverlayRemoval/lib/libCheatedMCOverlayRemoval.so"
        "$REPO_ROOT/source/AddNeutralPFOCovMat/lib/libAddNeutralPFOCovMat.so"
        "$REPO_ROOT/source/LeptonPairing/lib/libLeptonPairing.so"
        "$REPO_ROOT/source/HdecayMode/lib/libHdecayMode.so"
        "$REPO_ROOT/source/PreSelection/lib/libPreSelection.so"
        "$REPO_ROOT/source/FinalStateRecorder/lib/libFinalStateRecorder.so"
        "$MarlinML/lib64/libJetTaggers.so"
        "$VariablesForDeepMLFlavorTagger/lib/libVariablesForDeepMLFlavorTagger.so"
        "$BTaggingVariables/lib/libBTaggingVariables.so"
    )

    for lib in "${libs[@]}"; do
        if [[ ! -f "$lib" ]]; then
            echo "Error: Library <$lib> not found. Make sure it is compiled and the path is correct."
            return 1
        fi

        echo "Attaching library $(basename $lib)"
        export MARLIN_DLL=$MARLIN_DLL:"$lib"
    done

    #export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/LeptonErrorAnalysis/lib/libLeptonErrorAnalysis.so
    #export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/JetErrorAnalysis/lib/libJetErrorAnalysis.so
    #export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/ZHHKinfitProcessors/lib/libZHHKinfitProcessors.so
    #export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/Misclustering/lib/libMisclustering.so

    # MarlinReco + Legacy
    # export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/legacy/lib/libzhhll4j.so
}

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
            ZHH_INSTALL_DIR="./dependencies"
            return 0
            if [ ! -n "$argn" ]; then
                list_releases $os
                return 0
            elif [ -n "$argn" ] && [[ "$argn" =~ ^(almalinux|centos|ubuntu) ]]; then
                list_releases $argn
                return 0
            else
                echo "Unsupported OS $argn, aborting..."
                usage
                return 1
            fi
            ;;
        --install-dir|-d)
            if [ -z "$argn" ]; then
                echo "install-dir requires a non-empty argument"
                return 1
            else
                ZHH_INSTALL_DIR="$argn"
                return 0
            fi
            ;;
        -r)
            if [ -z "$argn" ]; then
                echo "release requires a non-empty argument"
                return 1
            else
                ZHH_K4H_RELEASE="$argn"
                return 0
            fi
            ;;
        --compile|-c)
            ZHH_COMMAND="compile"
            ;;
        *)
            eval "prev=\${$((i-1))}"
            if [ "$prev" != "-r" ]; then
                echo "Unknown argument $arg, it will be ignored"
                # usage
                # return 1
            fi
            ;;
    esac
done

#########################################

REPO_ROOT=$(readlink -f "$0")
REPO_ROOT=$(dirname "$REPO_ROOT")

if [[ -z "${MARLIN_DLL}" ]]; then
    source /cvmfs/sw.hsf.org/key4hep/setup.sh -r $ZHH_K4H_RELEASE
fi

if [[ -f ".env" && -z $ZHH_ENV_DOT ]]; then
    echo "Loading local environment file .env..."
    export $(grep -v '^#' .env | xargs)
    export ZHH_ENV_DOT=true
fi

if [[ -f ".env.sh" && -z $ZHH_ENV_DOT_SH ]]; then
    echo "Sourcing local sh file .env.sh..." 
    source .env.sh
    export ZHH_ENV_DOT_SH=true
fi

# Automatically find pytorch
if [[ -z "${TORCH_PATH}" ]]; then
    echo "Trying to find pytorch..."
    TORCH_PATH=$(dirname $(python -c 'import torch; print(f"{torch.__file__}")'))
fi

if [[ $CMAKE_PREFIX_PATH != *"torch/share/cmake"* ]]; then
    export CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}:${TORCH_PATH}/share/cmake
    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${TORCH_PATH}/lib
fi

if [[ "$ZHH_COMMAND" = "install" ]]; then
    echo "Attempting to install dependencies..."
    zhh_install $ZHH_INSTALL_DIR
    
    ZHH_COMMAND="compile"
fi

if [[ ! -d "$MarlinML" || ! -d "$VariablesForDeepMLFlavorTagger" || ! -d "$BTaggingVariables" ]]; then
    echo "MarlinML, VariablesForDeepMLFlavorTagger and BTaggingVariables must be set and point to valid directories. Use --install to download and compile them inside here."
    return 1
fi

if [[ "$ZHH_COMMAND" = "compile" ]]; then
    echo "Attempting to recompile dependencies..."
    zhh_recompile

    echo "Successfully compiled all dependencies and libraries"
fi

if [[ $MARLIN_DLL != *"libFinalStateRecorder"* ]]; then
    zhh_attach_marlin_dlls
fi