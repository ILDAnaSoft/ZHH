#!/bin/bash

# Meta information for preparing output structure and expected output files
TASK_NAME="MarlinTesting"
outputs=(AIDAFile_$suffix.root.root FinalStates_$suffix.root)

# Receive parameters
raw_file=${1}
suffix=${2}
env_file=${3}
job_id=${4}

# Prepare ZHH working environment
export $(grep -v '^#' "${env_file}" | xargs)

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

# Create output directories
mkdir -p "$TASK_ROOT/$TASK_NAME/logs"
mkdir -p "$TASK_ROOT/$TASK_NAME/output"

# Only execute Marlin if the job has not been run before
outputs_exist_all="True"
outputs_exist_any="False"

for output_file in ${outputs[*]}
do
    if [[ -f "$TASK_ROOT/$TASK_NAME/output/$output_file" ]]; then
        outputs_exist_any="True"
    else
        echo "Output $output_file does not exist. Marlin will be run..."
        outputs_exist_all="False"
        break
    fi
done

if [[ $outputs_exist_all = "False" ]]; then
    # Source environment
    if [[ -z "$ENV_SETUP_SCRIPT" ]]; then
        export ENV_SETUP_SCRIPT="$REPO_ROOT/setup.sh"
    fi

    load_env() {
        source "$ENV_SETUP_SCRIPT"
    }
    load_env

    # Run Marlin and transfer outputs
    echo "Running Marlin"
    Marlin "$REPO_ROOT/scripts/dev_flavortag_lcfiml.xml" --global.LCIOInputFiles=$raw_file --constant.ILDConfigDir=$ILD_CONFIG_DIR  --global.MaxRecordNumber=0 --constant.OutputSuffix=$suffix
    excode=$(exit 1)

    echo "Finished Marlin at $(date) with exit-code $excode"
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
            exit 1
        fi
    done
else
    echo "Job execution finished; outputs already exist"
fi

exit 0