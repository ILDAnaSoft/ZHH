#!/bin/bash

# Receive parameters
OUTPUT_BDIR=${1}
OUTPUT_BNAME=${2}
REPO_ROOT=${3}
raw_file=${4}

# Run Marlin and transfer outputs
cd $OUTPUT_BDIR
source $REPO_ROOT/setup.sh

echo "Running Marlin"
echo "Marlin $REPO_ROOT/scripts/dev_flavortag_compare.xml --global.LCIOInputFiles=$raw_file --global.MaxRecordNumber=0 --constant.OutputBaseName=$OUTPUT_BNAME --constant.ILDConfigDir=$REPO_ROOT/dependencies/ILDConfig --constant.ZHH_REPO_ROOT=$REPO_ROOT"
Marlin "$REPO_ROOT/scripts/dev_flavortag_compare.xml" --global.LCIOInputFiles=$raw_file --global.MaxRecordNumber=0 --constant.OutputBaseName=$OUTPUT_BNAME --constant.ILDConfigDir="$REPO_ROOT/dependencies/ILDConfig" --constant.ZHH_REPO_ROOT=$REPO_ROOT

exit 0