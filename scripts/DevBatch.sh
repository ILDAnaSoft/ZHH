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
STEERING_FILE="$SCRIPTDIR/prod.xml"

FILE1="$DATA_PATH/FastSimSGV/550-llhh-fast-perf/E550-Test.Pe2e2hh.Gwhizard-2_8_5.eL.pR.I404001.0.slcio"
NAME1="llhh_1"
NEVT1=100

FILE2="$DATA_PATH/FastSimSGV/550-llbb-fast-perf/E550_TDR.Pllbb_sl0.Gwhizard-3.1.4.eL.pR.slcio"
NAME2="llhh_2"
NEVT2=100

#FILE2="/data/dust/user/bliewert/zhh/FastSimSGV/550-llhh-fast-perf/E550-Test.Pe2e2qqh.Gwhizard-2_8_5.eL.pR.I404011.0.slcio"
#NAME2="llqqh"
#NEVT2=100

# how many jobs are spawned PER INPUT FILE
USE_CLUSTER_N_JOBS=200
USE_CLUSTER_BASEDIR="${DATA_PATH}/DevBatch"

# ---------------------------------------------------------------

if [ -z "$1" ] && [ -z "$2" ] && [ -z "$3" ]; then
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
    local STEERING_FILE="$5"

    if [ ! -f "$INFILE" ] || [ -z $OUTNAME ]; then
        echo "Either input file <$INFILE> does not exist or invalid OUTNAME chosen <$OUTNAME>"
        return 1
    fi

    if [ -z "$SKIPN" ] || [ "$SKIPN" = "0" ]; then
        SKIPN="1"
    fi

    echo "Processing input file <$INFILE> OUTNAME=$OUTNAME SKIPN=$SKIPN MAXN=$MAXN STEERING_FILE=$STEERING_FILE"

    local start=$(date +'%s')
    MarlinZHH "$STEERING_FILE" --global.LCIOInputFiles="$INFILE" --global.SkipNEvents=$SKIPN --global.MaxRecordNumber=$MAXN --constant.OutputBaseName=$OUTNAME > $OUTNAME.log 2>&1
    echo "$OUTNAME took $(($(date +'%s') - $start)) seconds"

    [ -f "${OUTNAME}_AIDA.root" ]
}

if [ "$USE_CLUSTER" = "y" ]; then
    if [ ! -z "$1" ] && [ ! -z "$2" ] && [ ! -z "$3" ] && [ ! -z "$4" ] && [ ! -z "$5" ] && [ ! -z "$6"  ]; then
        # this is a submitted job
        FILE=$1
        COUNTER=$2
        MAXN=$3
        STEERING_FILE=$4
        NAME=$5
        SKIPN=$6

        OUTNAME="$NAME.$COUNTER"

        runJob $FILE $OUTNAME $SKIPN $MAXN $STEERING_FILE

    else
        # create the job files, submit the jobs
        cd $SCRIPTDIR
        
        mkdir -p "$BASEDIR/logs"
        LOG_PATH=$(realpath "$BASEDIR/logs")

        rm -f DevBatch.sub
        cat > "DevBatch.sub" <<EOF
executable = $REPO_ROOT/scripts/DevBatch.sh

arguments = \$(file) \$(counter) \$(nmax) \$(steer) \$(name) \$(skipn)
should_transfer_files = True
transfer_executable = True

environment = "REPO_ROOT=$REPO_ROOT"
#max_retries = 3
Requirements = (Machine =!= LastRemoteHost)

log = $LOG_PATH/\$(name).\$(counter).log
output = $LOG_PATH/\$(name).\$(counter).out
error = $LOG_PATH/\$(name).\$(counter).err

transfer_output_files = ""

queue counter,name,steer,nmax,skipn,file from DevBatch.queue

EOF

        REWRITE_QUEUE_FILE="n"
        if [ -f "DevBatch.queue" ]; then
            read -p "> Re-write the queue file? y/n (n) " REWRITE_QUEUE_FILE

            if [ "$REWRITE_QUEUE_FILE" = "y" ]; then
                rm -f "DevBatch.queue"
            fi
        fi

        if [ "$REWRITE_QUEUE_FILE" = "y" ] || [ ! -f "DevBatch.queue" ]; then
            for i in `seq 0 $USE_CLUSTER_N_JOBS`
            do
                SKIPN=$(($NEVT1*$i))
                echo "$i,$NAME1,$STEERING_FILE,$NEVT1,$SKIPN,$FILE1" >> DevBatch.queue

                if [ ! -z "$FILE2" ]; then
                    SKIPN=$(($NEVT2*$i))
                    echo "$i,$NAME2,$STEERING_FILE,$NEVT2,$SKIPN,$FILE2" >> DevBatch.queue
                fi
            done
        fi

        read -p "> Submit jobs? (y) " yn
        if [ -z "$yn" ] || [ "$yn" = "y" ]; then
            condor_submit DevBatch.sub
        fi
    fi
else
    return 0
    # execute in the background
    { runJob $FILE1 $NAME1 0 $NEVT1 $STEERING_FILE; } &
    PID1=$!

    if [ ! -z "$NAME2" ]; then
        { runJob $FILE2 $NAME2 0 $NEVT2 $STEERING_FILE; } &
        PID2=$!
    fi

    if [ ! -z "$NAME2" ]; then
        wait $PID1 $PID2
    else
        wait $PID1
    fi

    cd $SCRIPTDIR
fi
