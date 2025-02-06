#!/bin/bash

# this file installs SGV into the directory given to it as first argument
# it should be sourced by you or whatever other file calls it
# usage: source sgv_install.sh <SGV_DIR> <LCIO_PATH>
# where LCIO_PATH is an optional second argument pointing to a user copy of LCIO

# this file has been tested with key4hep stack release 2025-01-28 to source it,
#   source /cvmfs/sw.hsf.org/key4hep/setup.sh -r "2025-01-28"
# remark: so far STDHEP is not working when setting up SGV using this file

# dependencies
export SGV_DIR="$1"
LCIO_PATH=$2

echo "Installing SGV into <$SGV_DIR>..."

LCIO_INTERFACE_VERSION="2.0.2"

if [[ ! -z $LCIO_PATH && -d $LCIO_PATH ]]; then
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"$LCIO/build/lib"
    export CPATH=$CPATH:"$LCIO/src/cpp/include":"$LCIO/src/cpp/include/pre-generated"
    export LCIO=$LCIO_PATH
fi

# abort on any error
# set -e

rm -rf $SGV_DIR
mkdir -p $SGV_DIR

cd $SGV_DIR
git clone https://gitlab.desy.de/mikael.berggren/sgv .
. samples/ld_preload_for_el9

# install stdhep support
# for now, stdhep support seems broken (why?) gfortran version etc?
if [[ "1" == "2" ]]; then
    cd $SGV_DIR/samples
    . install-stdhep

    # $STDHEP_DIR should now point to a valid directory
    if [[ ! -d "$STDHEP_DIR" ]]; then
        echo "$STDHEP_DIR not pointing to a valid directory"
        exit 1
    fi
fi

# install pythia6
(  
    cd $SGV_DIR/samples
    . install-pythia ../
)

# check if pythia has been installed correctly; the echo will fail if nothing is found
PYTHIA_LIB=$(find $SGV_DIR/v6_428/lib -name libpythia*.a)
echo $PYTHIA_LIB | grep .

export PYTHIA_DIR=$(dirname $PYTHIA_LIB)

# run SGV setup, now that the dependencies are set up
cd $SGV_DIR

# Continue without cernlib support? [Y/n]
# The root-config command exists. Do you want root-dependent features to be used ? (Y/n)
# The LCIO environment variable is set. Do you want LCIO-dependent features to be used ? (Y/n)
# (LCIO version) Which one to use? (Default = 2.0.2)
# install tests (Y/n) ?
# directory for tests ? (default ./tests)
# Make libraries (Y/n) ?
# Directory for libraries ? (default lib)
# Compile options ?
# lib Doesn't exist. Shall I create it ? (Y/n)

. ./install << ANSWERS
Y
Y
Y
$LCIO_INTERFACE_VERSION
Y
./tests
Y
lib
-DEXTREAD
Y

ANSWERS

# rebuild
# bash makesgvlibs lib

# enable lcio support
# see samples/lcio/00_README
cd $SGV_DIR/samples/lcio && . ./move_to_src $LCIO_INTERFACE_VERSION
cp $SGV_DIR/samples/lcio/usesgvlcio.F95 $SGV_DIR/tests

cd $SGV_DIR/tests

cresgvexe merge usesgvlcio
cresgvexe merge sgvuser

# enable extread support
cp $SGV_DIR/samples/sgv_extread.steer $SGV_DIR/tests
cp $SGV_DIR/samples/sgvopt.F95 $SGV_DIR/tests

cd $SGV_DIR/tests

cresgvexe merge sgvopt "-DEXTREAD"

# enable PFL calorimetry option
# samples/pflow/README
cd $SGV_DIR/samples/pflow
cp em_dc_5.dat  em_loss_5.dat  had_dc_5.dat  had_loss_5.dat \
   zaccon.F95 eflow_par_type.F95 sgv_pflow_pythia.steer $SGV_DIR/tests
cd $SGV_DIR/tests

rm -f sgv.steer

ln -s had_dc_5.dat fort.80
ln -s had_loss_5.dat fort.81
ln -s em_dc_5.dat fort.82
ln -s em_loss_5.dat fort.83

gfortran -c -I$SGV_LIB/mod eflow_par_type.F95
gfortran -c -I$SGV_LIB/mod zaccon.F95

cresgvexe merge sgvopt "" "" "" "" "eflow_par_type.o zaccon.o"

# now everything with lcio
cresgvexe merge usesgvlcio "-DEXTREAD" "" "" "" "eflow_par_type.o zaccon.o"

# prepare the steering file; use sgv_ild_lcio.steer as template file
# will be filled by Python helper class SGVSteeringModifier
cp $SGV_DIR/samples/lcio/sgv_ild_lcio_extread.steer $SGV_DIR/tests
ln -s $SGV_DIR/tests/sgv_ild_lcio_extread.steer $SGV_DIR/tests/sgv.steer

# testing
ln -s sgv.steer fort.17
# ln -s /pnfs/desy.de/ilc/prod/ilc/mc-2020/generated/550-Test/6f-test/E550-Test.P6f_yyxylv.Gwhizard-3_1_5.eR.pL.I410003_0.0.slcio $SGV_DIR/tests/test.lcio