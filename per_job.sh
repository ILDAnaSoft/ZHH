#!/bin/bash

# Receive parameters
raw_file=${1}
file_name=${2}
output_path=${3}

source /cvmfs/sw.hsf.org/key4hep/setup.sh
mkdir -p "${output_path}/logs"

# set testing to "N" when submitting jobs :)
testing="N"
full_path="${output_path}/${file_name}.slcio"
log_out="${output_path}/logs/${file_name}.marlin.out"
evt_count=0
file_exists="N"

if [ -f "$full_path" ]; then
    evt_count=$(lcio_event_counter "$full_path" | tail -n1)
    file_exists="Y"
fi

echo ""
echo "Dry-run     : $testing"
echo "File exists : $file_exists     (location=$full_path)"
echo "Event count : $evt_count"

if [ $file_exists != "N" ] || [ $evt_count != "10000" ]; then
    echo "Rescheduling: Y"
    echo ""

    if [ $testing = "N" ]; then
        echo "Going to execute Marlin, as either the output file does not exist or the found event count is not 10000"
        rm -f $full_path

        echo "Marlin will be executed and output will be written in realtime to $log_out"
        source /afs/desy.de/user/o/oterogom/project/anavenv/bin/activate
        Marlin /afs/desy.de/user/o/oterogom/project/condor2/preprocessing/steer_bk.xml --global.LCIOInputFiles=$raw_file --constant.OutputDirectory=$output_path --constant.OutputBaseName=$file_name --global.MaxRecordNumber=0 >> "$log_out"
    fi
else
    echo "Rescheduling: N"
    echo "Already done :)"
fi

exit 0