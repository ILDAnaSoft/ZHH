#!/bin/bash

function marlin_run() {
    local INPUT_FILE=$1
    local OUT_BASE_NAME=$2
    local EVENTS_SKIP=$3
    local EVENTS_MAX=$4

    echo "INPUT_FILE: $INPUT_FILE"
    echo "OUT_BASE_NAME: $OUT_BASE_NAME"
    echo "EVENTS_SKIP: $EVENTS_SKIP"
    echo "EVENTS_MAX: $EVENTS_MAX"
    echo "System info: $(uname -a)"

    if [[ -z $INPUT_FILE || -z $OUT_BASE_NAME || -z $EVENTS_SKIP || -z $EVENTS_MAX ]];
    then
        echo "Invalid inputs"
        exit 1
    fi

    echo "Starting Marlin at $(date)"
    MarlinZHH --global.LCIOInputFiles=$INPUT_FILE --constant.OutputBaseName=$OUT_BASE_NAME --global.SkipNEvents=$EVENTS_SKIP --global.MaxRecordNumber=$EVENTS_MAX
    echo "Finished Marlin at $(date)"

    sleep 3

    local resultFile="${OUT_BASE_NAME}_AIDA.root"

    if [[ -f "$resultFile" ]];
    then
        is_root_readable "$resultFile" FinalStates
        if [ $? -eq 0 ]; then
            rootrm "$resultFile":hEvtProcessingTime
            return 0
        else
            echo "File does not contain a readable TTree. Did Marlin not finish successfully?"
            return 1
        fi
    else
        echo "Result file $resultFile not found"
        return 1
    fi
}
marlin_run $@