#!/bin/bash

# Prepares the Docker image for the integration tests (see Dockerfile and tests/): makes the
# ZHH configurations available in the Python environment and generates and simulates a small
# sample of events, which is reused by the tests.
# CVMFS must be mounted, and install.sh must have been run.
# usage: bash shell/ci_prepare_sample.sh

set -eo pipefail

REPO_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
TAG="550-llbb-fast-perf"

cd "$REPO_ROOT"

if [ ! -d /cvmfs/sw.hsf.org/key4hep ]; then
    echo "CVMFS is not available. Content of /run:"
    ls -la /run
    exit 1
fi

# the environment scripts of key4hep do not expect set -e
set +e
source workflows/setup.sh
set -e

(
    # as in zhh_install_venv: the packages of the key4hep stack must not hide the ones of the requirements
    unset PYTHONPATH
    pip install -e .
)

# law run does not find any task without the index
law index --verbose

# DEBUG_NEVENTS: see workflows/analysis/configurations.py
# HEP_WORKFLOWS_DEFAULT_WORKFLOW: run the whole dependency tree locally, not on HTCondor
# OMP_NUM_THREADS: the tasks run in parallel; on a batch system they get one core each
export DEBUG_NEVENTS=100 HEP_WORKFLOWS_DEFAULT_WORKFLOW=local OMP_NUM_THREADS=1

law run FastSimSGV --tag="$TAG" --workers="$(nproc)"

ls "$DATA_PATH/WhizardEventGeneration/$TAG"/*.slcio "$DATA_PATH/FastSimSGV/$TAG"/*.slcio
echo "Sample for tag $TAG created"
