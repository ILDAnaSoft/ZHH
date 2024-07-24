#!/bin/bash

# Dependendies:
# -> $ILCSOFT_ROOT/MarlinReco/Analysis/SLDCorrection/lib/libSLDCorrection.so
# -> most ZHH processors

#while [ ! -z $CONDA_PREFIX ]; do conda deactivate; done

if ! command -v conda &> /dev/null
then
    echo "Making conda available (not sourcing any environment)..."
    source /afs/desy.de/user/b/bliewert/.zshrc
fi

if [[ -z "${MARLIN_DLL}" ]]; then
    # source /cvmfs/ilc.desy.de/key4hep/releases/2023-05-23/key4hep-stack/2023-05-24/x86_64-centos7-gcc12.3.0-opt/7emhu/setup.sh
    # source /cvmfs/sw.hsf.org/key4hep/setup.sh
    source /cvmfs/sw.hsf.org/key4hep/setup.sh -r 2023-11-23
    # source /cvmfs/ilc.desy.de/key4hep/setup.sh
fi

if [[ -z "${REPO_ROOT}" ]]; then
    export REPO_ROOT="/afs/desy.de/user/b/bliewert/public/MarlinWorkdirs/ZHH"
fi

if [[ -z "${ILCSOFT_ROOT}" ]]; then
    export ILCSOFT_ROOT="/afs/desy.de/user/b/bliewert/public/ILCSoft"
fi

is_root_readable() (
    if [ -e "/afs/desy.de/group/flc" ]; then
        local ON_NAF="true"
        local CONDA_ROOT="/nfs/dust/ilc/user/bliewert/miniconda3"
        local CONDA_ENV_NAME="graphjet_pyg"
    else
        local ON_NAF="false"
        local CONDA_ROOT="$HOME/miniforge3"
        local CONDA_ENV_NAME="py311"
    fi

    local PYTHONPATH=/afs/desy.de/user/b/bliewert/public/MarlinWorkdirs/pyhepcommon

    #echo "CONDA_DEFAULT_ENV: $CONDA_DEFAULT_ENV | CONDA_ENV_NAME: $CONDA_ENV_NAME"
    #if [ "$CONDA_DEFAULT_ENV" != "$CONDA_ENV_NAME" ]; then
    #if [[ "$($CONDA_ROOT/envs/$CONDA_ENV_NAME/bin/python -m site)" != *"pyhepcommon"* ]]; then
    #    echo "Activating conda $CONDA_ENV_NAME"
    #    conda activate $CONDA_ROOT/envs/$CONDA_ENV_NAME
        #&> /dev/null
    #fi

    #echo "PYTHONPATH: $PYTHONPATH"

    local root_tree=${2:-None}

    if [ "$root_tree" != "None" ]; then
        local root_tree="'$root_tree'"
    fi

    #echo "which python: $(which python)"
    #echo "which conda $(which conda)"
    
    local res=$($CONDA_ROOT/envs/$CONDA_ENV_NAME/bin/python -c "from phc import root_file_readable; print(root_file_readable('${1}', ${root_tree}))")
    
    if [ "$res" = "True" ]; then
        return 0
    else
        return 1
    fi
)

echo "Relative library path set to ${REPO_ROOT}"

if [[ $MARLIN_DLL != *"libFinalStateRecorder"* ]]; then
    # Starting July 2024, libnsl.so.1 cannot be found. They seem to be not available on batch nodes only, but are present on the local machines
    # As a temporary (?) workaround, we use a clone of the lib64 directory from the WGS node (they use nearly the same version)
    #export MARLIN_DLL=$(echo "$MARLIN_DLL" | sed "s~/cvmfs/ilc.desy.de/key4hep/releases/2023-05-23/pandoraanalysis/2.0.1/x86_64-centos7-gcc12.3.0-opt/oqkyr/lib/libPandoraAnalysis.so:~~g")
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/nfs/dust/ilc/user/bliewert/lib64
    
    # CheatedMCOverlayRemoval NOT working right now (segfaults)
    export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/CheatedMCOverlayRemoval/lib/libCheatedMCOverlayRemoval.so

    # Debugging LCFIPlus
    # 2024-03-10
    #export MARLIN_DLL=$(echo $MARLIN_DLL | sed -e "s#cvmfs/sw.hsf.org/key4hep/releases/2024-03-10/x86_64-centos7-gcc12.2.0-opt/lcfiplus/0.10.1-ff6lg4#root/public/DevLocal/LCFIPlus#g")
    
    # 2023-11-23
    #export MARLIN_DLL=$(echo $MARLIN_DLL | sed -e "s#cvmfs/sw.hsf.org/key4hep/releases/2023-11-23/x86_64-centos7-gcc12.2.0-opt/lcfiplus/0.10.1-z7amkm#root/public/DevLocal/LCFIPlus#g")    

    export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/AddNeutralPFOCovMat/lib/libAddNeutralPFOCovMat.so
    #export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/LeptonErrorAnalysis/lib/libLeptonErrorAnalysis.so
    export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/LeptonPairing/lib/libLeptonPairing.so
    #export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/HdecayMode/lib/libHdecayMode.so
    export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/PreSelection/lib/libPreSelection.so
    #export MARLIN_DLL=$MARLIN_DLL:$ILCSOFT_ROOT/MarlinReco/Analysis/SLDCorrection/lib/libSLDCorrection.so
    #export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/JetErrorAnalysis/lib/libJetErrorAnalysis.so
    #export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/ZHHKinfitProcessors/lib/libZHHKinfitProcessors.so
    #export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/Misclustering/lib/libMisclustering.so
    export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/FinalStateRecorder/lib/libFinalStateRecorder.so

    # MarlinReco + Legacy
    # export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/legacy/lib/libzhhll4j.so

    # Other dependencies
    #export MARLIN_DLL=$MARLIN_DLL:/afs/desy.de/user/b/bliewert/public/ILCSoft/Physsim/build/lib/libPhyssim.so
    #export MARLIN_DLL=$MARLIN_DLL:/afs/desy.de/user/b/bliewert/public/yradkhorrami/SLDecayCorrection/build/lib/libSLDecayCorrection.so
    #export LD_LIBRARY_PATH=$LCIO/lib64:/afs/desy.de/user/b/bliewert/public/ILCSoft/Physsim/lib:$LD_LIBRARY_PATH
fi