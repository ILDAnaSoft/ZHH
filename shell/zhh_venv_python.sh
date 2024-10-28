#!/bin/bash

if [[ ! -d "$REPO_ROOT" ]]; then
    REPO_ROOT=$(dirname $(dirname $(readlink -f "$0")))

    if [[ ! -d "$REPO_ROOT/zhh" || ! -d "$REPO_ROOT/source" ]]; then
        # The repository root was not found
        return 1
    fi
fi

if [[ ! -d "$REPO_ROOT/zhhvenv" ]]; then
    # Python environment zhhvenv is not available
    # Check the setup.sh --install
    return 1
fi

setupwrapper() {
    source $REPO_ROOT/setup.sh --force 2>&1 >/dev/null
}
setupwrapper

source $REPO_ROOT/zhhvenv/bin/activate

exec python "$@"