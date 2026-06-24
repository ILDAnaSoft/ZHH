#!/usr/bin/env bash

action() {    
    local shell_is_zsh="$( [ -z "${ZSH_VERSION}" ] && echo "false" || echo "true" )"
    local this_file="$( ${shell_is_zsh} && echo "${(%):-%x}" || echo "${BASH_SOURCE[0]}" )"
    local this_file="$( realpath "$this_file" )"
    local this_dir="$( cd "$( dirname "${this_file}" )" && pwd )"

    export REPO_ROOT=$( dirname "$this_dir" )

    source "$REPO_ROOT/setup.sh"
    source "$REPO_ROOT/$ZHH_VENV_NAME/bin/activate"

    export PYTHONPATH="$REPO_ROOT:${PYTHONPATH}"
    export LAW_HOME="${this_dir}/.law"
    export LAW_CONFIG_FILE="${this_dir}/law.cfg"
    export SH_ENVIRONMENT_FILE="$this_file"

    export MARLIN_RECO_STEERING_FILE="$REPO_ROOT/scripts/prod_reco_run.xml"
    export MARLIN_ANALYSIS_STEERING_FILE="$REPO_ROOT/scripts/prod_analysis_run.xml"

    source "$( law completion )" ""
}
action
