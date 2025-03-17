#!/bin/bash

# incoming arguments
OUTPUT_DIR=$1
JOB_NAME=$2
JOB_BASEDIR=$3

NMAX_PER_JOB=3600

if [[ -z $ZHH_SETUP ]]; then
    echo "ZHH not loaded. Call srczhh"
    exit 1
fi

# actual definition
# here, only files from /home/ilc/tianjp/generator/PostDBD/whizard3/toGrid/E550_6f_2l4q.slcio.list
readarray file_names < files.txt
nfiles=0
njobs=0
nexecute=0
ChunkGlobal=0

# keep track of indices in index.txt
rm -f index.txt

for INPUT_FILE in ${file_names[*]};
do
    file=$( basename ${INPUT_FILE} )
    dir=$( dirname ${INPUT_FILE} )
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
            
            sed -i -e "s|<INPUT_FILE>|$INPUT_FILE|g" definitions/$JOB_FILE
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