#!/bin/bash

function usage() {
    echo "Usage: source setup.sh [--install] [--install-dir=./install]"
    echo "       --install: downloads all dependencies, attempts to compile and install them as well as all libraries inside this repository"
    echo "       --install-dir: defaults to dependencies"
    echo "       --recompile: updates all dependencies. requires all paths (dependencies) to be set"
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

REPO_ROOT=$(readlink -f "$0")
REPO_ROOT=$(dirname "$REPO_ROOT")

if [[ -z "${MARLIN_DLL}" ]]; then
    source /cvmfs/sw.hsf.org/key4hep/setup.sh -r 2024-04-12

    # Uncomment the following line to let Python find py-torch
    #TORCH_PATH=$(dirname $(python -c 'import torch; print(f"{torch.__file__}")'))
    TORCH_PATH="/cvmfs/sw.hsf.org/key4hep/releases/2024-03-10/x86_64-almalinux9-gcc11.3.1-opt/py-torch/2.2.1-le2nos/lib/python3.10/site-packages/torch"    
    export CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}:${TORCH_PATH}/share/cmake
    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${TORCH_PATH}/lib
fi

if [[ -f ".env" ]]; then
    export $(grep -v '^#' .env | xargs)
fi

if [[ -f ".env.sh" ]]; then
    source .env.sh
fi

if [[ "$1" = "--install" ]]; then
    INSTALL_DIR="./dependencies"

    if [[ "$2" = "--install-dir" && -n "$3" ]]; then
        if [ -d "$3" ]; then
            echo "install-dir must be empty"
            return 1
        fi
        
        INSTALL_DIR="$3"
    fi

    if [[ -f ".env" ]]; then
        read -p "You wish to install the dependencies, but an .env file which would be overwritten already exists. Do you wish to back it up to .env.bck and continue? (y)" yn
        if [[ "$yn" = "y" ]]; then
            rm -f .env.bck
            mv .env .env.bck
        else
            return 1
        fi
    fi

    mkdir -p $INSTALL_DIR && cd $INSTALL_DIR

    git clone https://gitlab.desy.de/ilcsoft/MarlinML
    git clone https://gitlab.desy.de/ilcsoft/variablesfordeepmlflavortagger
    git clone https://gitlab.desy.de/ilcsoft/btaggingvariables

    export MarlinML="$(pwd)/MarlinML"
    export VariablesForDeepMLFlavorTagger="$(pwd)/variablesfordeepmlflavortagger"
    export BTaggingVariables="$(pwd)/btaggingvariables"
    export ZHH_RECOMPILE=true

    # Save directories to .env
    cat >> "$REPO_ROOT/.env" <<EOF
MarlinML="$MarlinML"
VariablesForDeepMLFlavorTagger="$VariablesForDeepMLFlavorTagger"
BTaggingVariables="$BTaggingVariables"

EOF
fi

if [[ ! -d "$MarlinML" || ! -d "$VariablesForDeepMLFlavorTagger" || ! -d "$BTaggingVariables" ]]; then
    echo "MarlinML, VariablesForDeepMLFlavorTagger and BTaggingVariables must be set and point to valid directories. Use --install to download and compile them inside here."
    return 1
fi

if [[ "$1" = --recompile || "$ZHH_RECOMPILE" = true ]]; then
    # Compile the ZHH processors

    cd $REPO_ROOT
    source compile_from_scratch.sh

    # Compile the ML and helper libraries
    for module_to_compile in "$MarlinML" "$VariablesForDeepMLFlavorTagger" "$BTaggingVariables"
    do
        compile_pkg $module_to_compile && echo "+++ Successfully compiled $module_to_compile +++" || { echo "!!! Error [$?] while trying to compile $module_to_compile !!!"; cd $REPO_ROOT; return 1; }
    done

    cd $REPO_ROOT
    echo "+++ Successfully compiled all dependencies and libraries +++"
fi

if [[ $MARLIN_DLL != *"libFinalStateRecorder"* ]]; then
    # Starting July 2024, libnsl.so.1 cannot be found. They seem to be not available on batch nodes only, but are present on the local machines
    # As a temporary (?) workaround, we use a clone of the lib64 directory from the WGS node (they use nearly the same version)
    # export MARLIN_DLL=$(echo "$MARLIN_DLL" | sed "s~/cvmfs/ilc.desy.de/key4hep/releases/2023-05-23/pandoraanalysis/2.0.1/x86_64-centos7-gcc12.3.0-opt/oqkyr/lib/libPandoraAnalysis.so:~~g")
    # export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/nfs/dust/ilc/user/bliewert/lib64

    # Debugging LCFIPlus
    # 2024-03-10
    #export MARLIN_DLL=$(echo $MARLIN_DLL | sed -e "s#cvmfs/sw.hsf.org/key4hep/releases/2024-03-10/x86_64-centos7-gcc12.2.0-opt/lcfiplus/0.10.1-ff6lg4#root/public/DevLocal/LCFIPlus#g")
    
    # 2023-11-23
    #export MARLIN_DLL=$(echo $MARLIN_DLL | sed -e "s#cvmfs/sw.hsf.org/key4hep/releases/2023-11-23/x86_64-centos7-gcc12.2.0-opt/lcfiplus/0.10.1-z7amkm#root/public/DevLocal/LCFIPlus#g")    

    export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/CheatedMCOverlayRemoval/lib/libCheatedMCOverlayRemoval.so
    export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/AddNeutralPFOCovMat/lib/libAddNeutralPFOCovMat.so
    
    export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/LeptonPairing/lib/libLeptonPairing.so
    export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/HdecayMode/lib/libHdecayMode.so
    export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/PreSelection/lib/libPreSelection.so
    export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/FinalStateRecorder/lib/libFinalStateRecorder.so

    # MarlinML + helpers
    export MARLIN_DLL=$MARLIN_DLL:$MARLIN_ML_DIR/lib64/libJetTaggers.so
    export MARLIN_DLL=$MARLIN_DLL:$VARIABLESFORDEEPMLFLAVORTAGGER/lib/libVariablesForDeepMLFlavorTagger.so
    export MARLIN_DLL=$MARLIN_DLL:$BTAGGINGVARIABLES/lib/libBTaggingVariables.so

    #export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/LeptonErrorAnalysis/lib/libLeptonErrorAnalysis.so
    #export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/JetErrorAnalysis/lib/libJetErrorAnalysis.so
    #export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/ZHHKinfitProcessors/lib/libZHHKinfitProcessors.so
    #export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/Misclustering/lib/libMisclustering.so

    # MarlinReco + Legacy
    # export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/legacy/lib/libzhhll4j.so
fi