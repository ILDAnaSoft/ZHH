#!/usr/bin/env bash

action() {
    # Configuration
    export REPO_ROOT="/afs/desy.de/user/b/bliewert/public/MarlinWorkdirs/ZHH"
    export DATA_PATH="/nfs/dust/ilc/user/bliewert/zhh"

    # All this should be automatic (except for CONDA_ROOT)

    if [ -e "/afs/desy.de/group/flc" ]; then
        local ON_NAF="true"
        local CONDA_ROOT="/nfs/dust/ilc/user/bliewert/miniconda3"
        local CONDA_ENV_NAME="graphjet_pyg"
    else
        local ON_NAF="false"
        local CONDA_ROOT="$HOME/miniforge3"
        local CONDA_ENV_NAME="py311"
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
