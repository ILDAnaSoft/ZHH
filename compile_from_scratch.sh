#!/bin/bash
# If called with the keyword keep (source compile_from_scratch.sh keep), only make install will be called on the processors

action(){
    local RED='\033[0;31m'
    local GREEN='\033[0;32m'
    local BLUE='\033[0;34m'
    local NC='\033[0m'
    local module_to_compile    
    local delete_existing=$( [[ "$1" == "keep" || "$2" == "keep" || "$3" == "keep" ]] && echo "False" || echo "True" )
    local isdebug=$( [[ "$1" == "debug" || "$2" == "debug" || "$3" == "debug" ]] && echo "True" || echo "False" )
    local startdir=$(pwd)
    local offline=$( [[ "$1" == "offline" || "$2" == "offline" || "$3" == "offline" ]] && echo "True" || echo "False" )
    local skipdeps=$( [[ "$1" == "skipdeps" || "$2" == "skipdeps" || "$3" == "skipdeps" ]] && echo "True" || echo "False" )
    local modules

    if [[ $skipdeps == "True" ]]; then
        modules=("source")
    else
        modules=("$MarlinMLFlavorTagging $MarlinReco $MarlinKinfit $LCFIPlus $Physsim source")
    fi

    compile_pkg ()
    {        
        (
            cd $1
            if [[ $delete_existing = "True" ]]; then
                echo "Deleting any existing build directory..."
                rm -rf build
            fi

            if [[ $offline != "True" ]]; then
                git pull
            fi

            mkdir -p build
            cd build

            if [[ $delete_existing = "True" || ! -f Makefile  ]]; then
                cmake -DCMAKE_CXX_STANDARD=17 ..
            fi
            
            if [[ $isdebug = "True" ]]; then
                make install -j 4 || exit $?
            else
                rm -f build.log
                make install -j 4 &> build.log
            
                local retval=$?

                if [[ $retval -ne 0 ]]; then
                    echo -e "$RED!!! Error [$retval] while trying to compile $1 !!!$NC"

                    cat build.log
                    rm -f build.log

                    exit $retval
                fi

                rm -f build.log
            fi
            
            cd ../..
        )
    }

    for module_to_compile in ${modules[@]};
    do
        echo -e "$BLUE+++ Compiling $(basename $module_to_compile)... +++$NC"
        if [[ -d $module_to_compile ]]; then
            compile_pkg $module_to_compile && echo -e "$GREEN+++ Successfully compiled $module_to_compile +++$NC" || { echo -e "$RED!!! Error [$?] while trying to compile $module_to_compile !!!$NC" && cd $start_dir && return 1; }
        else
            echo -e "$RED!!! Error: $module_to_compile not found !!!$NC"
        fi
    done

    echo -e "$GREEN+++ Successfully compiled ZHH and dependencies +++$NC"

    unset compile_pkg

    return 0
}
action $@