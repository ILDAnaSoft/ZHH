#!/usr/bin/bash

if [ -z "$LD_LIBRARY_PATH" ] || [[ ! $LD_LIBRARY_PATH =~ "Physsim" ]]; then
    echo "Fatal error: This script must be executed with Physsim in LD_LIBRARY_PATH. Did you source a key4hep stack/the ZHH environment?"
    return 1
else
    echo "Compiling PhyssimWrapper Python lib..."
fi

PWD=$(pwd)
( cd build && ) || ( echo "Failed to compile" && cd $PWD && cd .. )