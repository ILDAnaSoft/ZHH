#!/usr/bin/env bash

action() {
    conda activate py311

    local shell_is_zsh="$( [ -z "${ZSH_VERSION}" ] && echo "false" || echo "true" )"
    local this_file="$( ${shell_is_zsh} && echo "${(%):-%x}" || echo "${BASH_SOURCE[0]}" )"
    local this_dir="$( cd "$( dirname "${this_file}" )" && pwd )"

    export LAW_HOME="${this_dir}/.law"
    export LAW_CONFIG_FILE="${this_dir}/law.cfg"

    export ANALYSIS_PATH="${this_dir}"
    export DATA_PATH="/nfs/dust/ilc/user/bliewert/zhh"
    export REPO_ROOT="/afs/desy.de/user/b/bliewert/public/ZHH"

    source "$( law completion )" ""
}
action
