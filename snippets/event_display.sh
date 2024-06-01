#!/bin/bash

# Tested using /cvmfs/ilc.desy.de/key4hep/releases/089d775cf2
# e.g.: event_display.sh /pnfs/desy.de/ilc/prod/ilc/mc-2020/ild/rec/500-TDR_ws/hh/ILD_l5_o1_v02/v02-02-03/00015739/000/rv02-02-03.sv02-02-03.mILD_l5_o1_v02.E500-TDR_ws.I403001.Pe2e2hh.eL.pR.n000_001.d_rec_00015739_265.slcio

echo "Showing event from ${1}"

ced2go -d "$lcgeo_DIR/ILD/compact/ILD_l5_v02/ILD_l5_v02.xml" ${1}