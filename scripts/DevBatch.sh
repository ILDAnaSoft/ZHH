#!/usr/bin/bash

if [ ! -z "$REPO_ROOT" ] && [ -d "$REPO_ROOT" ]; then
    source $REPO_ROOT/setup.sh
elif ! source ../setup.sh; then
    echo "Fatal error: Could not source ZHH setup.sh"
    exit 1
fi

if [ ! -d "$REPO_ROOT/scripts" ]; then
    echo "Fatal error: Incorrect REPO_ROOT <$REPO_ROOT> resolved"
    exit 1
fi

SCRIPTDIR="$REPO_ROOT/scripts"
BASEDIR="${SCRIPTDIR}/DevBatch"

FILE1="/data/dust/user/bliewert/zhh/FastSimSGV/550-llhh-fast-perf/E550-Test.Pe2e2hh.Gwhizard-2_8_5.eL.pR.I404001.0.slcio"
NAME1="llhh"
NEVT1=100

FILE2="/data/dust/user/bliewert/zhh/FastSimSGV/550-llbb-fast-perf/E550_TDR.Pllbb_sl0.Gwhizard-3.1.4.eL.pR.slcio"
NAME2="llbb"
NEVT2=100

#FILE2="/data/dust/user/bliewert/zhh/FastSimSGV/550-llhh-fast-perf/E550-Test.Pe2e2qqh.Gwhizard-2_8_5.eL.pR.I404011.0.slcio"
#NAME2="llqqh"
#NEVT2=100

# how many jobs are spawned PER INPUT FILE
USE_CLUSTER_N_JOBS=20
USE_CLUSTER_BASEDIR="${DATA_PATH}/DevBatch"

# ---------------------------------------------------------------

if [ -z "$1" ] && [ -z "$2" ]; then
    USE_CLUSTER="NO"
    read -p "> Submit via cluster? y/n (n) " USE_CLUSTER

    if [ "$USE_CLUSTER" = "y" ]; then
        BASEDIR="$USE_CLUSTER_BASEDIR"
    fi

    rm -rf $BASEDIR/*.root $BASEDIR/*.slcio $BASEDIR/*.log $BASEDIR/*.json
    mkdir -p $BASEDIR
else
    # this is a cluster job
    USE_CLUSTER="y"
    BASEDIR="$USE_CLUSTER_BASEDIR"
fi

cd $BASEDIR

function runJob(){
    local INFILE="$1"
    local OUTNAME="$2"
    local SKIPN=$3
    local MAXN=$4

    if [ ! -f "$INFILE" ] || [ -z $OUTNAME ]; then
        echo "Either input file <$INFILE> does not exist or invalid OUTNAME chosen <$OUTNAME>"
        return 1
    fi

    if [ -z "$SKIPN" ] || [ "$SKIPN" = "0" ]; then
        SKIPN="1"
    fi

    echo "Processing input file <$INFILE> OUTNAME=$OUTNAME SKIPN=$SKIPN MAXN=$MAXN"

    local start=$(date +'%s')
    MarlinZHH --global.LCIOInputFiles="$INFILE" --global.SkipNEvents=$SKIPN --global.MaxRecordNumber=$MAXN --constant.OutputBaseName=$OUTNAME > $OUTNAME.log 2>&1
    echo "$OUTNAME took $(($(date +'%s') - $start)) seconds"

    [ -f "${OUTNAME}_AIDA.root" ]
}

if [ "$USE_CLUSTER" = "y" ]; then
    if [ ! -z "$1" ] && [ ! -z "$2"  ]; then
        # this is a submitted job
        NAME=$1
        COUNTER=$2

        if [ $NAME = $NAME1 ]; then
            FILE=$FILE1
            MAXN=$NEVT1
        elif [ $NAME = $NAME2 ]; then
            FILE=$FILE2
            MAXN=$NEVT2
        else
            echo "Fatal error: Unknown name <$NAME>"
            exit 1
        fi

        SKIPN=$(($MAXN*$COUNTER))
        OUTNAME="$NAME.$COUNTER"

        runJob $FILE $OUTNAME $SKIPN $MAXN

    else
        # create the job files, submit the jobs
        cd $SCRIPTDIR
        
        mkdir -p "$BASEDIR/logs"
        LOG_PATH=$(realpath "$BASEDIR/logs")

        rm -f DevBatch.sub
        cat > "DevBatch.sub" <<EOF
executable = $REPO_ROOT/scripts/DevBatch.sh

arguments = \$(name) \$(counter)
should_transfer_files = True
transfer_executable = True

environment = "REPO_ROOT=$REPO_ROOT"
#max_retries = 3
Requirements = (Machine =!= LastRemoteHost)

log = $LOG_PATH/\$(name).\$(counter).log
output = $LOG_PATH/\$(name).\$(counter).out
error = $LOG_PATH/\$(name).\$(counter).err

transfer_output_files = ""

queue name,counter from DevBatch.queue

EOF

        rm -f DevBatch.queue

        for i in `seq 0 $USE_CLUSTER_N_JOBS`
        do
            echo "$NAME1,$i
$NAME2,$i" >> DevBatch.queue
        done

        read -p "> Submit jobs? (y) " yn
        if [ -z "$yn" ] || [ "$yn" = "y" ]; then
            condor_submit DevBatch.sub
        fi
    fi
else
    # execute in the background
    { runJob $FILE1 $NAME1 0 $NEVT1; } &
    PID1=$!
    { runJob $FILE2 $NAME2 0 $NEVT2; } &
    PID2=$!

    wait $PID1 $PID2

    cd $SCRIPTDIR
fi
