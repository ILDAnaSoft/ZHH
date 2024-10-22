#!/usr/bin/env bash

function zhh_activate_conda() {
    # We rely on all important environment variables existing inside $REPO_ROOT/.env
    if [ -z "$REPO_ROOT" ]; then
        echo "Trying to infer REPO_ROOT..."

        REPO_ROOT=$(realpath "${BASH_SOURCE[${#BASH_SOURCE[@]} - 1]}")
        REPO_ROOT=$(dirname "$REPO_ROOT")
        REPO_ROOT=$(dirname "$REPO_ROOT")

        if [ -d "$REPO_ROOT" ]; then
            echo "Found REPO_ROOT at <$REPO_ROOT>"
        else
            echo "REPO_ROOT not found, aborting..."
            return 1
        fi
    fi

    if [[ -z $DATA_PATH || ! -d "${DATA_PATH}" ]]; then
        if [[ -f "${REPO_ROOT}/.env" ]]; then
            echo "Loading local environment file .env..."
            export $(grep -v '^#' "${REPO_ROOT}/.env" | xargs)
        fi
    fi

    if [[ -f "${REPO_ROOT}/.env.sh" ]]; then
        echo "Sourcing local sh file .env.sh..." 
        source "${REPO_ROOT}/.env.sh"
    fi

    if [[ ! -d "$CONDA_ROOT" || ! -d "$CONDA_ROOT/envs/$CONDA_ENV" ]]; then
        echo "Either invalid conda root <$CONDA_ROOT> or invalid environment <$CONDA_ENV> given, aborting..."
        return 1
    fi

    # Make sure to initialize conda if is has not been before
    if [[ -z $CONDA_SHLVL ]]; then
        echo "conda seems to not have been initialized before. initializing..."
        #source "${CONDA_ROOT}/etc/profile.d/conda.sh"

        local shell_name="$( [ -z "${ZSH_VERSION}" ] && echo "bash" || echo "zsh" )"
        eval "$($CONDA_ROOT/bin/conda shell.$shell_name hook)"
    fi

    conda activate $CONDA_ROOT/envs/$CONDA_ENV
}