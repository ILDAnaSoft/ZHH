#!/usr/bin/bash

INPUT_FILE=$1
TARGET_FILE=$2

if [[ ! -z $REPO_ROOT || ! -d $REPO_ROOT ]]; then
    srczhh
fi

# variables depending on SGV setup
if [[ ! -z $3 ]]; then
    SGV_PRODUCED_FILE=$3
else
    SGV_PRODUCED_FILE=sgvout.slcio
fi

if [[ ! -z $4 ]]; then
    SGV_EXECUTABLE=$4
else
    SGV_EXECUTABLE=$SGV_DIR/tests/usesgvlcio.exe
fi

if [[ ! -z $5 ]]; then
    SGV_INPUT_FILE=$5
else
    SGV_INPUT_FILE=input.slcio
fi

if [[ -z $INPUT_FILE || -z $TARGET_FILE || -z $SGV_PRODUCED_FILE || -z $SGV_EXECUTABLE || -z $SGV_INPUT_FILE || -z $SGV_DIR ]]; then
    echo "At least one necessary parameter is missing"
    echo $REPO_ROOT
    echo $INPUT_FILE
    echo $TARGET_FILE
    echo $SGV_PRODUCED_FILE
    echo $SGV_EXECUTABLE
    echo $SGV_INPUT_FILE
    echo $SGV_DIR

    exit 1
fi

source $SGV_DIR/sgvenv.sh \
&& echo "SRC=$INPUT_FILE DST=$TARGET_FILE" \
&& cp -R $(dirname $SGV_EXECUTABLE)/* . \
&& ln -s "$INPUT_FILE" "$SGV_INPUT_FILE" \
&& rm -f fort.51 && ln -s sgv_geo.inp fort.51 \
&& echo "Starting SGV at $(date)" \
&& ( ./$(basename $SGV_EXECUTABLE) && echo "Finished SGV at $(date)" \
&& echo "Moving to destination..." \
&& mv "$SGV_PRODUCED_FILE" "$TARGET_FILE" )