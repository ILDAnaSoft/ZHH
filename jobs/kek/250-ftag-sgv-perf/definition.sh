#!/bin/bash

# incoming arguments
OUTPUT_DIR=$1
JOB_NAME=$2
JOB_BASEDIR=$3

# actual definition
dir="/gpfs/group/ilc/grid/storm/prod/ilc/mc-2020/generated/250-SetA/flavortag"
files=$( ls $dir )
nfiles=0
njobs=0

for file in $( echo $files );
do
    INPUT_FILE="$dir/$file"
    OUT_BASE_DIR="$OUTPUT_DIR/$JOB_NAME"
    TARGET_FILE="$OUT_BASE_DIR/$file"

    # check for existing targets
    if [[ ! -f $TARGET_FILE ]]; then        
        # create job file
        JOB_FILE="$file.sh"

        cp payload.template definitions/$JOB_FILE
        
        sed -i -e "s|<INPUT_FILE>|$INPUT_FILE|g" definitions/$JOB_FILE
        sed -i -e "s|<TARGET_FILE>|$TARGET_FILE|g" definitions/$JOB_FILE
        sed -i -e "s|<OUT_BASE_DIR>|$OUT_BASE_DIR|g" definitions/$JOB_FILE

        # add to pack.job
        echo "bash $JOB_BASEDIR/definitions/$JOB_FILE" >> pack.job

        njobs=$((njobs + 1 ))
    else
        echo "File <$TARGET_FILE> already exists. Skipping."
    fi

    nfiles=$((nfiles + 1 ))
done

echo "----------------------------------------------------------"
echo "Prepared $njobs jobs given a total of $nfiles input files "