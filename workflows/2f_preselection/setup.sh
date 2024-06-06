#!/usr/bin/env bash

action() {
    export ILCSOFT_ROOT="$HOME/public/ILCSoft"
    export REPO_ROOT="$HOME/public/MarlinWorkdirs/ZHH"
    export DATA_PATH="/nfs/dust/ilc/user/bliewert/zhh"

    local CONDA_ENV_NAME="py311"
    local ON_NAF="false"

    if [[ $( cat /etc/hostname ) == *"desy.de"* ]]; then
        ON_NAF="true"
        local CONDA_ROOT="/nfs/dust/ilc/user/bliewert/miniconda3"
    else
        local CONDA_ROOT="$HOME/miniforge3"
        
    fi
    conda activate $CONDA_ROOT/envs/$CONDA_ENV_NAME

    local shell_is_zsh="$( [ -z "${ZSH_VERSION}" ] && echo "false" || echo "true" )"
    local this_file="$( ${shell_is_zsh} && echo "${(%):-%x}" || echo "${BASH_SOURCE[0]}" )"
    local this_dir="$( cd "$( dirname "${this_file}" )" && pwd )"

    export PYTHONPATH="${this_dir}:${PYTHONPATH}"
    export LAW_HOME="${this_dir}/.law"
    export LAW_CONFIG_FILE="${this_dir}/law.cfg"

    export ANALYSIS_PATH="${this_dir}"

    source "$( law completion )" ""
}
action
