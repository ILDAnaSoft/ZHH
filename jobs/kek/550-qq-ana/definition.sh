#!/bin/bash

# incoming arguments
OUTPUT_DIR=$1
JOB_NAME=$2
JOB_BASEDIR=$3

NMAX_PER_JOB=3600

if [[ -z $ZHH_SETUP ]]; then
    srczhh
fi

# actual definition
dir="/group/ilc/users/bliewert/jobresults/550-hh-sgv"
files=($( ls $dir/*.Pq* ))
nfiles=0
njobs=0
ChunkGlobal=0

# keep track of indices in index.txt
rm -f index.txt

for file in ${files[*]};
do
    file=$( basename $file )
    INPUT_FILE="$dir/$file"
    OUT_BASE_NAME="${file%.*}"
    OUT_BASE_DIR="$OUTPUT_DIR/$JOB_NAME"
    TARGET_FILE="$OUT_BASE_DIR/$OUT_BASE_NAME.slcio"

    nEvents=$( lcio_event_counter $INPUT_FILE )
    Pointer=0
    ChunkLocal=0

    # check for existing targets
    if [[ ! -f $TARGET_FILE ]]; then
        while [ $Pointer -lt $nEvents ];
        do
            # create job file
            JOB_FILE="$file-$ChunkLocal.sh"

            cp payload.template definitions/$JOB_FILE
            
            sed -i -e "s|<INPUT_FILE>|$INPUT_FILE|g" definitions/$JOB_FILE
            sed -i -e "s|<OUT_BASE_NAME>|$OUT_BASE_NAME-$ChunkLocal|g" definitions/$JOB_FILE
            sed -i -e "s|<OUT_BASE_DIR>|$OUT_BASE_DIR|g" definitions/$JOB_FILE
            sed -i -e "s|<ZHH_REPO_ROOT>|$ZHH_REPO_ROOT|g" definitions/$JOB_FILE
            sed -i -e "s|<EVENTS_MAX>|$NMAX_PER_JOB|g" definitions/$JOB_FILE
            sed -i -e "s|<EVENTS_SKIP>|$Pointer|g" definitions/$JOB_FILE

            # add to pack.job and index.txt
            echo "bash $JOB_BASEDIR/definitions/$JOB_FILE" >> pack.job
            echo "$ChunkGlobal $INPUT_FILE $OUT_BASE_NAME-$ChunkLocal $ChunkLocal $Pointer $((Pointer + $NMAX_PER_JOB)) " >> index.txt

            njobs=$((njobs + 1 ))
            Pointer=$((Pointer + $NMAX_PER_JOB))
            ChunkGlobal=$((ChunkGlobal + 1))
            ChunkLocal=$((ChunkLocal + 1))
        done
    else
        echo "File <$TARGET_FILE> already exists. Aborting due to index error."
        exit 1
    fi

    nfiles=$((nfiles + 1 ))
done

echo "----------------------------------------------------------"
echo "Prepared $njobs jobs given a total of $nfiles input files "