#!/bin/bash

if [[ -z "${MARLIN_DLL}" ]]; then
    source /cvmfs/ilc.desy.de/key4hep/releases/2023-05-23/key4hep-stack/2023-05-24/x86_64-centos7-gcc12.3.0-opt/7emhu/setup.sh
    # source /cvmfs/ilc.desy.de/key4hep/setup.sh
fi

export ILD_ANASOFT_ZHH=$(pwd)

echo "Using current directory as relative path for libraries, which is ${ILD_ANASOFT_ZHH}"

#export MARLIN_DLL=$MARLIN_DLL:$ILD_ANASOFT_ZHH/source/AddNeutralPFOCovMat/lib/libAddNeutralPFOCovMat.so
#export MARLIN_DLL=$MARLIN_DLL:$ILD_ANASOFT_ZHH/source/LeptonErrorAnalysis/lib/libLeptonErrorAnalysis.so
#export MARLIN_DLL=$MARLIN_DLL:$ILD_ANASOFT_ZHH/source/CheatedMCOverlayRemoval/lib/libCheatedMCOverlayRemoval.so
#export MARLIN_DLL=$MARLIN_DLL:$ILD_ANASOFT_ZHH/source/LeptonPairing/lib/libLeptonPairing.so
#export MARLIN_DLL=$MARLIN_DLL:$ILD_ANASOFT_ZHH/source/HdecayMode/lib/libHdecayMode.so
#export MARLIN_DLL=$MARLIN_DLL:$ILD_ANASOFT_ZHH/source/PreSelection/lib/libPreSelection.so
#export MARLIN_DLL=$MARLIN_DLL:$ILD_ANASOFT_ZHH/source/JetErrorAnalysis/lib/libJetErrorAnalysis.so
#export MARLIN_DLL=$MARLIN_DLL:$ILD_ANASOFT_ZHH/source/ZHHKinfitProcessors/lib/libZHHKinfitProcessors.so
#export MARLIN_DLL=$MARLIN_DLL:$ILD_ANASOFT_ZHH/source/Misclustering/lib/libMisclustering.so

# MarlinReco + Legacy
# export MARLIN_DLL=$MARLIN_DLL:$ILD_ANASOFT_ZHH/source/legacy/lib/libzhhll4j.so

# Other dependencies
#export MARLIN_DLL=$MARLIN_DLL:/afs/desy.de/user/b/bliewert/public/ILCSoft/Physsim/build/lib/libPhyssim.so
#export MARLIN_DLL=$MARLIN_DLL:/afs/desy.de/user/b/bliewert/public/yradkhorrami/SLDecayCorrection/build/lib/libSLDecayCorrection.so
#export LD_LIBRARY_PATH=$LCIO/lib64:/afs/desy.de/user/b/bliewert/public/ILCSoft/Physsim/lib:$LD_LIBRARY_PATH
