#!/bin/bash

#source /cvmfs/ilc.desy.de/sw/x86_64_gcc82_centos7/v02-02-03/init_ilcsoft.sh
#source /afs/desy.de/project/ilcsoft/sw/x86_64_gcc82_centos7/v02-02-03/init_ilcsoft.sh
#source /afs/desy.de/user/b/bliewert/public/MarlinWorkdirs/TestPhyssimK4H/setup.sh
source /cvmfs/ilc.desy.de/key4hep/setup.sh

echo "Using current directory as relative path for libraries, which is ${PWD}"

export MARLIN_DLL=$MARLIN_DLL:/afs/desy.de/user/b/bliewert/public/ILCSoft/Physsim/build/lib/libPhyssim.so:$PWD/source/AddNeutralPFOCovMat/lib/libAddNeutralPFOCovMat.so:$PWD/source/LeptonErrorAnalysis/lib/libLeptonErrorAnalysis.so:$PWD/source/CheatedMCOverlayRemoval/lib/libCheatedMCOverlayRemoval.so:$PWD/source/LeptonPairing/lib/libLeptonPairing.so:$PWD/source/HdecayMode/lib/libHdecayMode.so:$PWD/source/PreSelection/lib/libPreSelection.so:$PWD/source/JetErrorAnalysis/lib/libJetErrorAnalysis.so:$PWD/source/ZHHKinfitProcessors/lib/libZHHKinfitProcessors.so:$PWD/source/Misclustering/lib/libMisclustering.so
export MARLIN_DLL=$MARLIN_DLL:/afs/desy.de/user/b/bliewert/public/yradkhorrami/SLDecayCorrection/build/lib/libSLDecayCorrection.so
export LD_LIBRARY_PATH=$LCIO/lib64:/afs/desy.de/user/b/bliewert/public/ILCSoft/Physsim/lib:$LD_LIBRARY_PATH
