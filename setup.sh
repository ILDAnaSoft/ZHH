#!/bin/bash

# Dependendies:
# -> $ILCSOFT_ROOT/MarlinReco/Analysis/SLDCorrection/lib/libSLDCorrection.so
# -> most ZHH processors

if [[ -z "${MARLIN_DLL}" ]]; then
    # source /cvmfs/ilc.desy.de/key4hep/releases/2023-05-23/key4hep-stack/2023-05-24/x86_64-centos7-gcc12.3.0-opt/7emhu/setup.sh
    # source /cvmfs/sw.hsf.org/key4hep/setup.sh
    source /cvmfs/sw.hsf.org/key4hep/setup.sh -r 2024-03-10
    # source /cvmfs/ilc.desy.de/key4hep/setup.sh
fi

if [[ -z "${REPO_ROOT}" ]]; then
    export REPO_ROOT="/afs/desy.de/user/b/bliewert/public/MarlinWorkdirs/ZHH"
fi

if [[ -z "${ILCSOFT_ROOT}" ]]; then
    export ILCSOFT_ROOT="/afs/desy.de/user/b/bliewert/public/ILCSoft"
fi

echo "Relative library path set to ${REPO_ROOT}"

if [[ $MARLIN_DLL != *"libFinalStateRecorder"* ]]; then
    # Starting July 2024, libnsl.so.1 cannot be found. They seem to be not available on batch nodes only, but are present on the local machines
    # As a temporary (?) workaround, we use a clone of the lib64 directory from the WGS node (they use nearly the same version)
    #export MARLIN_DLL=$(echo "$MARLIN_DLL" | sed "s~/cvmfs/ilc.desy.de/key4hep/releases/2023-05-23/pandoraanalysis/2.0.1/x86_64-centos7-gcc12.3.0-opt/oqkyr/lib/libPandoraAnalysis.so:~~g")
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/nfs/dust/ilc/user/bliewert/lib64

    export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/AddNeutralPFOCovMat/lib/libAddNeutralPFOCovMat.so
    #export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/LeptonErrorAnalysis/lib/libLeptonErrorAnalysis.so
    export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/CheatedMCOverlayRemoval/lib/libCheatedMCOverlayRemoval.so
    export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/LeptonPairing/lib/libLeptonPairing.so
    export MARLIN_DLL=$MARLIN_DLL:$REPO_ROOT/source/HdecayMode/lib/libHdecayMode.so
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