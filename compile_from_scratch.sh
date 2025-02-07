#!/bin/bash
# If called with the keyword keep (source compile_from_scratch.sh keep), only make install will be called on the processors

action(){
    local RED='\033[0;31m'
    local GREEN='\033[0;32m'
    local BLUE='\033[0;34m'
    local NC='\033[0m'
    local module_to_compile    
    local delete_existing=$( [[ "$1" == "keep" || "$2" == "keep" ]] && echo "False" || echo "True" )
    local startdir=$(pwd)

    compile_pkg ()
    {        
        (
            cd $1
            if [[ $delete_existing = "True" ]]; then
                echo "Deleting any existing build directory..."
                rm -rf build
            fi

            mkdir -p build
            cd build

            if [[ $delete_existing = "True" || ! -f Makefile  ]]; then
                cmake -DCMAKE_CXX_STANDARD=17 ..
            fi
            
            make install -j 4 && cd ../..
        )
    }

    for module_to_compile in "source" "$MarlinMLFlavorTagging" "$MarlinReco"
    do
        echo -e "$BLUE+++ Compiling $(basename $module_to_compile)... +++$NC"
        if [[ -d $module_to_compile ]]; then
            compile_pkg $module_to_compile && echo -e "$GREEN+++ Successfully compiled $module_to_compile +++$NC" || { echo -e "$RED!!! Error [$?] while trying to compile $module_to_compile !!!$NC" && cd $start_dir && return 1; }
        else
            echo -e "$RED!!! Error: $module_to_compile not found !!!$NC"
        fi
    done

    echo -e "$GREEN+++ Successfully compiled ZHH dependencies +++$NC"

    unset compile_pkg

    return 0
}
action $1
