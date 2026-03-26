#!/bin/bash

# incoming arguments
OUTPUT_DIR=$1
JOB_NAME=$2
JOB_BASEDIR=$3

OUT_BASE_DIR="$OUTPUT_DIR/$JOB_NAME"

if [ -z $ZHH_SETUP ]; then
    srczhh
fi

njobs=0

while IFS=, read -ra inputs; do
    ## Do something with ${arr0]}, ${arr[1]} and ${arr[2]}
    ENERGY=${inputs[0]}
    PROCESS=${inputs[1]}
    CIRCE2_FILE=${inputs[2]}
    HADRONIZATION_ACTIVE=${inputs[3]}

    echo $ENERGY $PROCESS
    OUT_BASE_NAME=$PROCESS

    JOB_FILE="$PROCESS.sh"
    JOB_SINDARIN="$PROCESS.sin"

    cp zz_h0.sin "definitions/$JOB_SINDARIN"
    cp payload.template "definitions/$JOB_FILE"

    sed -i -e "s|<ENERGY>|$ENERGY|g" definitions/$JOB_SINDARIN
    sed -i -e "s|<PROCESS>|$PROCESS|g" definitions/$JOB_SINDARIN
    sed -i -e "s|<CIRCE2_FILE>|$CIRCE2_FILE|g" definitions/$JOB_SINDARIN
    sed -i -e "s|<NEVENTS>|100000|g" definitions/$JOB_SINDARIN
    sed -i -e "s|<HADRONIZATION_ACTIVE>|$HADRONIZATION_ACTIVE|g" definitions/$JOB_SINDARIN

    sed -i -e "s|<OUT_BASE_NAME>|$OUT_BASE_NAME|g" definitions/$JOB_FILE
    sed -i -e "s|<OUT_BASE_DIR>|$OUT_BASE_DIR|g" definitions/$JOB_FILE
    sed -i -e "s|<ZHH_REPO_ROOT>|$ZHH_REPO_ROOT|g" definitions/$JOB_FILE
    sed -i -e "s|<SINDARIN_FILE>|$JOB_BASEDIR/definitions/$JOB_FILE|g" definitions/$JOB_FILE

    echo "bash $JOB_BASEDIR/definitions/$JOB_FILE" >> pack.job
    njobs=$((njobs + 1 ))

done < inputs.dat

return 1

for file in ${file_names[*]};
do
    file=$( basename $file )
    PROCESS=$
    INPUT_FILE="$dir/$file"
    OUT_BASE_NAME="${file%.*}"
    OUT_BASE_DIR="$OUTPUT_DIR/$JOB_NAME"

    nEvents=$( lcio_event_counter $INPUT_FILE )
    Pointer=0
    ChunkLocal=0

    # check for existing targets
    while [ $Pointer -lt $nEvents ];
    do
        TARGET_FILE="$OUT_BASE_DIR/$OUT_BASE_NAME-$ChunkLocal.slcio"
        if [[ ! -f $TARGET_FILE ]]; then
            # create job file
            JOB_FILE="$file-$ChunkLocal.sh"

            cp payload.template definitions/$JOB_FILE
            
            sed -i -e "s|<PROCESS>|$PROCESS|g" definitions/$JOB_FILE
            sed -i -e "s|<OUT_BASE_NAME>|$OUT_BASE_NAME-$ChunkLocal|g" definitions/$JOB_FILE
            sed -i -e "s|<OUT_BASE_DIR>|$OUT_BASE_DIR|g" definitions/$JOB_FILE
            sed -i -e "s|<ZHH_REPO_ROOT>|$ZHH_REPO_ROOT|g" definitions/$JOB_FILE
            sed -i -e "s|<EVENTS_MAX>|$NMAX_PER_JOB|g" definitions/$JOB_FILE
            sed -i -e "s|<EVENTS_SKIP>|$Pointer|g" definitions/$JOB_FILE

            echo "bash $JOB_BASEDIR/definitions/$JOB_FILE" >> pack.job
            nexecute=$((nexecute + 1 ))
        else
            echo "File <$TARGET_FILE> already exists. No job will be scheduled for it."
        fi

        # add to pack.job and index.txt
        echo "$ChunkGlobal $INPUT_FILE $OUT_BASE_NAME-$ChunkLocal $ChunkLocal $Pointer $((Pointer + $NMAX_PER_JOB)) " >> index.txt

        njobs=$((njobs + 1 ))
        Pointer=$((Pointer + $NMAX_PER_JOB))
        ChunkGlobal=$((ChunkGlobal + 1))
        ChunkLocal=$((ChunkLocal + 1))

        #echo "Prepared job $njobs"
        #if [ $njobs -eq 200 ]; then
        #    break 2
        #fi
    done
    
    nfiles=$((nfiles + 1 ))
done

echo "---------------------------------------------------------------------------------"
echo "Prepared $njobs jobs ($nexecute to execute) given a total of $nfiles input files "