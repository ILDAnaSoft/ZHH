#!/bin/bash

# Receive parameters
raw_file=${1}
suffix=${2}
env_file=${3}
job_id=${4}
CPID_NAME=${5}
TASK_NAME=${6}

# Meta information for preparing output structure and expected output files
outputs=(AIDAFile_$suffix.root.root FT_$suffix.slcio)

# Load job environment variables
export $(grep -v '^#' "${env_file}" | xargs)

# Print status
echo "Running task $TASK_NAME with parameters"
echo "  raw_file: $raw_file"
echo "  suffix: $suffix"
echo "  env_file: $env_file"
echo "  job_id: $job_id"
echo "  CPID_NAME: $CPID_NAME"
echo ""
echo "Starting $(date)"

if [[ ! -d $REPO_ROOT ]]; then
    exit "Critical error: REPO_ROOT is not set or does not point to a valid directory"
fi

# Create output directories
OUTPUT_BASEDIR="$TASK_ROOT/$TASK_NAME/output"
mkdir -p "$TASK_ROOT/$TASK_NAME/logs"
mkdir -p "$OUTPUT_BASEDIR"

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

    # Initialize temp working dir
    TMP_DIR="TMP-$job_id-$(basename $raw_file)"
    TMP_FULL="$OUTPUT_BASEDIR/$TMP_DIR"

    rm -rf $TMP_FULL
    mkdir -p $TMP_FULL
    cd $TMP_FULL

    #ln -s /afs/desy.de/group/flc/pool/ueinhaus/ILDConfig_v02-03-02/StandardConfig/production/HighLevelReco HighLevelReco

    # Run Marlin and transfer outputs
    echo "Running Marlin"
    Marlin "$REPO_ROOT/scripts/dev_flavortag_v2.xml" --global.LCIOInputFiles=$raw_file --global.MaxRecordNumber=0 --constant.OutputSuffix=$suffix --constant.ILDConfigDir=$ILD_CONFIG_DIR

    echo "Finished Marlin at $(date)"
    echo "Directory contents: $(ls)"

    sleep 5

    # Transfer outputs
    ROOT_OUTPUT="AIDAFile_$suffix.root"
    ROOT_TTREE="Jets"
    is_root_readable "$ROOT_OUTPUT" "$ROOT_TTREE"
    if [[ $? -ne 0 ]]; then
        echo "Root TTree <$ROOT_TTREE> not found. Job failed!"
        exit 2
    fi

    for output_file in ${outputs[*]}
    do
        if [[ -f "$output_file" ]]; then
            echo "Transferring output file $output_file"

            rm -f "$TASK_ROOT/$TASK_NAME/output/$output_file"
            mv "$output_file" "$TASK_ROOT/$TASK_NAME/output/$output_file" && echo "Done transferring $output_file" || echo "Failed to move output file $output_file"
        else
            echo "Failed to find output file $output_file"
            exit 1
        fi
    done

    cd $OUTPUT_BASEDIR
    rm -rf $TMP_FULL
else
    echo "Job execution finished; outputs already exist"
fi

exit 0