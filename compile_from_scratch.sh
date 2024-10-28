#!/bin/bash
# If called with the keyword keep (source compile_from_scratch.sh keep), only make install will be called on the processors

action(){
    local RED='\033[0;31m'
    local GREEN='\033[0;32m'
    local NC='\033[0m'
    local module_to_compile    
    local delete_existing=$( [[ "$1" == "keep" || "$2" == "keep" ]] && echo "False" || echo "True" )

    compile_pkg ()
    {
        echo "Compiling $1..."
        cd $1
        if [[ $delete_existing == "True" ]]; then
            echo "Deleting any existing build directory..."
            rm -rf build
        fi

        mkdir -p build
        cd build
	cmake -DCMAKE_CXX_STANDARD=17 ..
	#cmake -C $ILCSOFT/ILCSoft.cmake ..
        make install || { cd ../.. ; return 1; }
        cd ../..
    }

    cd source

    for module_to_compile in CheatedMCOverlayRemoval ChargedPFOCorrection AddNeutralPFOCovMat LeptonPairing HdecayMode JetErrorAnalysis JetTaggingComparison LeptonErrorAnalysis PreSelection ZHHKinfitProcessors Misclustering
    do
        compile_pkg $module_to_compile && echo "${GREEN}+++ Successfully compiled $module_to_compile +++${NC}" || { echo "${RED}!!! Error [$?] while trying to compile $module_to_compile !!!${NC}"; cd ..; return 1; }
    done
    cd ..

    echo "${GREEN}+++ Successfully compiled ZHH projects +++${NC}"

    unset compile_pkg

    return 0
}
action $1
