#!/bin/bash

# this file installs SGV into the directory given to it as first argument
# it should be sourced by you or whatever other file calls it
# usage: source sgv_install.sh <SGV_DIR>

# this file has been tested with
# - key4hep stack release 2025-01-28 and
# - SGV release e781bad09ef30595ed1db0343145eebbf5d00a1a
# to source this key4hep stack,
#   source /cvmfs/sw.hsf.org/key4hep/setup.sh -r "2025-01-28"
# remark 1: the LCIO version supplied by key4hep will be used per default
# remark 2: so far STDHEP is not working when setting up SGV using this file
#   this seems to be related with some dependency in the key4hep stack, as it
#   at least compiles with 2024-03-10 (maybe the gfortran version?)

# dependencies
export SGV_DIR="$1"

echo "Installing SGV into <$SGV_DIR>..."

LCIO_INTERFACE_VERSION="2.0.2"

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


rm -rf $SGV_DIR/src/sgvlcio/*
cp -R $SGV_DIR/samples/lcio/src-$LCIO_INTERFACE_VERSION/* $SGV_DIR/src/sgvlcio
cd $SGV_DIR/src/sgvlcio
rm -rf -- ..?* .[!.]*

cp $SGV_DIR/samples/lcio/usesgvlcio.F95 $SGV_DIR/tests

cd $SGV_DIR/tests

cresgvexe merge usesgvlcio
cresgvexe merge sgvuser

# enable extread support
cp $SGV_DIR/samples/sgv_extread.steer $SGV_DIR/tests
cp $SGV_DIR/samples/sgvopt.F95 $SGV_DIR/tests

cd $SGV_DIR/tests

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

cresgvexe merge sgvopt "-DEXTREAD" "" "" "" "eflow_par_type.o zaccon.o"

# now everything with lcio
cresgvexe merge usesgvlcio "-DEXTREAD" "" "" "" "eflow_par_type.o zaccon.o"

# prepare the steering file; use sgv_ild_lcio.steer as template file
# will be filled by Python helper class SGVSteeringModifier
cp $SGV_DIR/samples/lcio/sgv_ild_lcio_extread.steer $SGV_DIR/tests

# link steering and geometry file to fortran units 17 and 51 (hardcoded by SGV)
ln -s -f $SGV_DIR/tests/sgv_ild_lcio_extread.steer $SGV_DIR/tests/sgv.steer
ln -s -f $SGV_DIR/tests/sgv.steer $SGV_DIR/tests/fort.17
ln -s -f $SGV_DIR/tests/sgv_geo.inp $SGV_DIR/tests/fort.51

# set some defaults
sed -i -e "s|MAXEV = 100|MAXEV = 999999|g" sgv.steer
sed -i -e "s|GENERATOR_INPUT_TYPE = 'STDH'|GENERATOR_INPUT_TYPE = 'LCIO'|g" sgv.steer
sed -i -e "s|INPUT_FILENAMES = '\*.stdhep'|INPUT_FILENAMES = 'input.slcio'|g" sgv.steer
sed -i -e "s|!   CALO_TREATMENT = 'PERF'|   CALO_TREATMENT = 'PFL '|g" sgv.steer
sed -i -e "s|!  FILENAME = 'sgvout.slcio'|  FILENAME = 'sgvout.slcio'|g" sgv.steer

# the default uses sgvout.slcio as output file
# read all input

# ln -s /pnfs/desy.de/ilc/prod/ilc/mc-2020/generated/550-Test/6f-test/E550-Test.P6f_yyxylv.Gwhizard-3_1_5.eR.pL.I410003_0.0.slcio $SGV_DIR/tests/test.lcio