#!/bin/bash                                                                                                                                                             
#if [ -z ${ILCSOFT+x} ]
#then
#    echo "ILCSOFT is unset"
wrapper(){
    echo running wrapper
    source  /afs/desy.de/group/flc/pool/jtorndal/zhh/ZHH/setup.sh
}

wrapper

#else echo "ILCSOFT is set to '$ILCSOFT'"
#fi
outdir=/data/dust/user/jtorndal/ZHHAnalysis

#local steering_file=${1:-"$REPO_ROOT/scripts/prod.xml"}
commandargs="${0} ${1} ${2} ${3} ${4} ${5} ${6} ${7} ${8} ${9} ${10} ${11} ${12} ${13}"
timestamp=$(date +%F_%T)
day=$(date -I)
commit=$(git rev-parse --short HEAD)
if [[ $(git diff --stat) != '' ]]; then
  dirty='dirty'
else
  dirty='clean'
fi

mkdir -p "$outdir/logbook/"
echo "${timestamp} ${commit} ${dirty} ${commandargs}" >> "$outdir/logbook/$day"
Marlin /afs/desy.de/group/flc/pool/jtorndal/zhh/ZHH/scripts/zhhanalysis.xml --constant.ILDConfigDir="$ILD_CONFIG_DIR" --constant.ZHH_REPO_ROOT="$REPO_ROOT" --constant.OutputDirectory="${outdir}/${10}" --constant.OutputBaseName="${1}" --global.MaxRecordNumber="${2}" --global.SkipNEvents="${3}" --global.LCIOInputFiles="${4}" --constant.CMSEnergy="${5}" --constant.ISRPzMax="${6}" --constant.NumberOfHiggs="${7}" --constant.NumberOfJets="${8}" --constant.RunKinfit="${9}" --constant.WhichSignature="${10}" --constant.errorflowconfusion="${11}" --constant.propagateerrorflowconfusion="${12}" --constant.useneutralpfocorrection="${13}"
# Marlin /afs/desy.de/group/flc/pool/jtorndal/zhh/ZHH/scripts/zhhanalysis.xml --constant.ILDConfigDir="$ILD_CONFIG_DIR" --constant.ZHH_REPO_ROOT="$REPO_ROOT" --constant.OutputDirectory="/data/dust/user/bliewert/group/${10}" --constant.OutputBaseName="${1}" --global.MaxRecordNumber="${2}" --global.SkipNEvents="${3}" --global.LCIOInputFiles="${4}" --constant.CMSEnergy="${5}" --constant.ISRPzMax="${6}" --constant.NumberOfHiggs="${7}" --constant.NumberOfJets="${8}" --constant.RunKinfit="${9}" --constant.WhichSignature="${10}" --constant.errorflowconfusion="${11}" --constant.propagateerrorflowconfusion="${12}"

