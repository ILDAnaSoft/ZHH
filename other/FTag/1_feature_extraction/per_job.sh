#!/bin/bash

# Receive parameters
raw_file=${1}
suffix=${2}
env_file=${3}
job_id=${4}

# Prepare ZHH working environment
export $(grep -v '^#' "${env_file}" | xargs)

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
mkdir -p "$TASK_ROOT/$TASK_NAME/logs"
mkdir -p "$TASK_ROOT/$TASK_NAME/output"

# Expected output files
outputs=(AIDAFile_$suffix.root.root FT_$suffix.slcio)

# Only execute Marlin if the job has not been run before
outputs_exist_all="True"
outputs_exist_any="False"

for output_file in ${outputs[*]}
do
    if [[ -f "$TASK_ROOT/$TASK_NAME/$output_file" ]]; then
        outputs_exist_any="True"
    else
        outputs_exist_all="False"
    fi
done

if [[ $outputs_exist_all = "False" ]]; then

    # Run Marlin and transfer outputs
    echo "Running Marlin"
    Marlin "$REPO_ROOT/scripts/dev_flavortag.xml" --global.LCIOInputFiles=$raw_file --constant.ILDConfigDir=$ILD_CONFIG_DIR  --global.MaxRecordNumber=0 --constant.OutputSuffix=$suffix --constant.RunInference=false

    echo "Finished Marlin at $(date)"
    echo "Directory contents: $(ls)"

    sleep 5

    # Transfer outputs
    for output_file in ${outputs[*]}
    do
        if [[ -f "$output_file" ]]; then
            echo "Transferring output file $output_file"

            rm -rf "$TASK_ROOT/$TASK_NAME/output/$output_file"
            mv "$output_file" "$TASK_ROOT/$TASK_NAME/output/$output_file" && echo "Done transferring $output_file" || echo "Failed to move output file $output_file"
        else
            echo "Failed to find output file $output_file"
        fi
    done
fi