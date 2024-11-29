#!/bin/bash

# Receive parameters
raw_file=${1}
suffix=${2}
env_file=${3}
job_id=${4}

# Meta information for preparing output structure
TASK_NAME="FeatureExtraction"

# Print status
echo "Running task $TASK_NAME with parameters"
echo "  raw_file: $raw_file"
echo "  suffix: $suffix"
echo "  env_file: $env_file"
echo "  job_id: $job_id"
echo ""
echo "Starting $(date)"

# Prepare ZHH working environment
export $(grep -v '^#' "${env_file}" | xargs)

if [[ ! -d $REPO_ROOT ]]; then
    exit "Critical error: REPO_ROOT is not set or does not point to a valid directory"
fi

if [[ -z "$ENV_SETUP_SCRIPT" ]]; then
    export ENV_SETUP_SCRIPT="$REPO_ROOT/setup.sh"
fi

load_env() {
    source "$ENV_SETUP_SCRIPT"
}
load_env

# Create output directories
mkdir -p "$FT_DATA_PATH/$TASK_NAME/logs"
mkdir -p "$FT_DATA_PATH/$TASK_NAME/output"

# Only execute Marlin if the job has not been run before
if [[ ! -f "$FT_DATA_PATH/$TASK_NAME/output/AIDAFile_$suffix.root" ]]; then

    # Run Marlin and transfer outputs
    echo "Running Marlin"
    Marlin "$REPO_ROOT/scripts/dev_flavortag.xml" --global.LCIOInputFiles=$raw_file --constant.ILDConfigDir=$ILD_CONFIG_DIR  --constant.MaxRecordNumber=0 --constant.OutputSuffix=$suffix --constant.RunInference=false
    sleep 5
    (mv "FT_$suffix.slcio" "$FT_DATA_PATH/$TASK_NAME/output/FT_$suffix.slcio" && mv "AIDAFile_$suffix.root" "$FT_DATA_PATH/$TASK_NAME/output/AIDAFile_$suffix.root") || echo "Failed transfering either AIDA or Marlin output"

    # Copy logs
    rm -rf "$FT_DATA_PATH/$TASK_NAME/logs/$job_id.err" "$FT_DATA_PATH/logs/$job_id.out" "$FT_DATA_PATH/logs/$job_id.log"

    cp $job_id.err "$FT_DATA_PATH/$TASK_NAME/logs"
    cp $job_id.out "$FT_DATA_PATH/$TASK_NAME/logs"
    cp $job_id.log "$FT_DATA_PATH/$TASK_NAME/logs"

fi