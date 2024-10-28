#!/bin/bash

REPO_ROOT="/afs/desy.de/user/b/bliewert/public/MarlinWorkdirs/ZHH"

setupwrapper() { source $REPO_ROOT/setup.sh 2>&1 >/dev/null; }
setupwrapper && source $REPO_ROOT/zhhvenv/bin/activate && exec python $@