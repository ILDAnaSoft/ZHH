#!/usr/bin/env bash

# Script to setup key4hep nightly envrionment that allows to use (py)torch, onnx and Marlin
# together with DD4hep.
#
#

source /cvmfs/sw.hsf.org/key4hep/releases/2024-04-12/x86_64-almalinux9-gcc11.3.1-opt/key4hep-stack/2024-04-12-bwtdjs/setup.sh
TORCH_PATH=/cvmfs/sw.hsf.org/key4hep/releases/2024-03-10/x86_64-almalinux9-gcc11.3.1-opt/py-torch/2.2.1-le2nos/lib/python3.10/site-packages/torch
#source /cvmfs/sw-nightlies.hsf.org/key4hep/setup.sh

export MARLIN_DLL="/afs/desy.de/user/u/ueinhaus/pool/MarlinReco_v01-35/lib/libMarlinReco.so.1.35.0":$MARLIN_DLL

#TORCH_PATH=$(dirname $(python -c 'import torch; print(f"{torch.__file__}")'))
#TORCH_PATH="/cvmfs/sw-nightlies.hsf.org/key4hep/releases/2024-09-21/x86_64-almalinux9-gcc14.2.0-opt/py-torch/2.4.1-xaazey/lib/python3.11/site-packages/torch"
export CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}:${TORCH_PATH}/share/cmake
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${TORCH_PATH}/lib

export MARLIN_DLL="/afs/desy.de/group/flc/pool/bliewert/MarlinWorkdirs/ZHH/dependencies/MarlinMLFlavorTagging/lib/libMarlinMLFlavorTagging.so:$MARLIN_DLL"

#export MARLIN_DLL=$(echo "$MARLIN_DLL" | sed -e "s|/cvmfs/sw-nightlies.hsf.org/key4hep/releases/2024-11-28/x86_64-almalinux9-gcc14.2.0-opt/marlinmlflavortagging/902342aa6adca9af9e14e34da396886e238cc417_develop-6ehaku/lib/libMarlinMLFlavorTagging.so:|/afs/desy.de/group/flc/pool/bliewert/MarlinWorkdirs/ZHH/dependencies/MarlinMLFlavorTagging/lib/libMarlinMLFlavorTagging.so:|g")
export MARLIN_DLL="/afs/desy.de/group/flc/pool/bliewert/MarlinWorkdirs/ZHH/source/JetTaggingComparison/lib/libJetTaggingComparison.so":"$MARLIN_DLL"