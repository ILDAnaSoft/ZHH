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

# there are two resulting setup:
# 1st: in $SGV_DIR/tests and does the following:
# - read 999999 (i.e. all) events
# - use LCIO input
# - events are read from input.slcio in the same directory as the executable
# - use PFL calorimetry option with ILD defaults
# - write output to sgvout.slcio
#
# 2nd: in $SGV_DIR/tests_beamcal_500GeV does the same as 1st, but additionally:
# - compiles with ZTUGEO from samples/usercalo, implementing beamcal efficiencies fitted from ILD simulation at 500 GeV
# - set PRIMARY_VERTEX_SIM = .TRUE. in default steering file, sgv.steer

# for batch jobs, it is sufficient to source sgvenv.sh, copy the tests directory
# to the worker node, prepare input.slcio via symlink and then run usesgvlcio

# if you want to compile SGV with debug options, after running below script, run
# . ./makesgvlibs lib debug
# cd tests && cresgvexe merge usesgvlcio "-DEXTREAD debug" "" "" "" "eflow_par_type.o zaccon.o"

sgv_install_action () {
    if [ ! -z $1 ]; then
        export SGV_DIR=$1
    fi

    if [ -z "$LCIO" ]; then
        echo "LCIO env variable must be set. Make sure to source a key4hep stack"
        return 1
    fi

    local LCIO_INTERFACE_VERSION="2.0.2"

    # abort on any error

    if [ -z "$SGV_DIR" ]; then
        if [ -f "$PWD/samples/sgv_extread.steer" ]; then
            echo "Using SGV installation in this directory"
            export SGV_DIR="$PWD"

            if [ -e "$SGV_DIR/.installed" ]; then
                echo "SGV has been installed already. Execute bash clean in order to avoid conflicts."
                return 1
            fi
        else
            echo "No SGV_DIR given"
            return 2
        fi
    else
        echo "Installing SGV into <$SGV_DIR>..."

        rm -rf $SGV_DIR && mkdir -p $SGV_DIR && cd $SGV_DIR
        git clone https://gitlab.desy.de/mikael.berggren/sgv .
    fi

    # required on Alma9/RHEL9, and doesn't hurt on other systems
    if [[ $(find /usr/lib64/ -name "libblas.so*" -type f | wc -l) = 1 && $(find /usr/lib64/ -name "liblapack.so*" -type f | wc -l) = 1 ]]; then
        . samples/ld_preload_for_el9
    else
        echo "Either libblas.so or liblapack.so could not be found. If you don't fix this issue, SGV will not work correctly. You can either build it yourself or try to install it via package manager."
        
        local yn=""
        read -p "Do you wish to continue? y/n (n) " yn
        local yn=${yn:-"n"}

        if [ "$yn" != "y" ]; then
            return 3
        fi
    fi

    # prepare stdhep support;
    # as stated above, this is disabled to support the newest key4hep stack versions
    # you can enable it manually by setting the following to "1"
    if [ "1" = "2" ]; then
        cd $SGV_DIR/samples
        . install-stdhep

        # $STDHEP_DIR should now point to a valid directory
        if [ ! -d "$STDHEP_DIR" ]; then
            echo "$STDHEP_DIR not pointing to a valid directory"
            return 4
        fi
    fi

    # install pythia6
    if [ ! -d "$SGV_DIR/v6_428/lib" ]; then
        (  
            cd $SGV_DIR/samples
            . install-pythia ../
        )
    fi

    # check if pythia has been installed correctly; the echo will fail if nothing is found
    local PYTHIA_LIB=$(find $SGV_DIR/v6_428/lib -name libpythia*.a)
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

    # enable lcio support
    # see samples/lcio/00_README
    rm -rf $SGV_DIR/src/sgvlcio/*
    cp -R $SGV_DIR/samples/lcio/src-$LCIO_INTERFACE_VERSION/* $SGV_DIR/src/sgvlcio
    cd $SGV_DIR/src/sgvlcio
    rm -rf -- ..?* .[!.]*

    cp "$SGV_DIR/samples/lcio/usesgvlcio.F95" "$SGV_DIR/tests"
    cd "$SGV_DIR/tests"

    cresgvexe merge usesgvlcio
    cresgvexe merge sgvuser

    # 1st setup

    # enable extread support
    cp "$SGV_DIR/samples/sgv_extread.steer" "$SGV_DIR/tests"
    cp "$SGV_DIR/samples/sgvopt.F95" "$SGV_DIR/tests"

    # enable PFL calorimetry option
    # samples/pflow/README
    cd "$SGV_DIR/samples/pflow"
    cp em_dc_5.dat  em_loss_5.dat  had_dc_5.dat  had_loss_5.dat \
    zaccon.F95 eflow_par_type.F95 sgv_pflow_pythia.steer "$SGV_DIR/tests"
    cd "$SGV_DIR/tests"

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
    cp "$SGV_DIR/samples/lcio/sgv_ild_lcio_extread.steer" "$SGV_DIR/tests"

    # link steering and geometry file to fortran units 17 and 51 (hardcoded by SGV)
    ln -s -f "$SGV_DIR/tests/sgv_ild_lcio_extread.steer" "$SGV_DIR/tests/sgv.steer"
    ln -s -f "$SGV_DIR/tests/sgv.steer" "$SGV_DIR/tests/fort.17"
    ln -s -f "$SGV_DIR/tests/sgv_geo.inp" "$SGV_DIR/tests/fort.51"

    # set defaults: process all events, read in LCIO, use PFL calorimetry, output to sgvout.slcio, output full MC reco-truth link
    sed -i -e "s|MAXEV = 100|MAXEV = 999999|g" sgv.steer
    sed -i -e "s|GENERATOR_INPUT_TYPE = 'STDH'|GENERATOR_INPUT_TYPE = 'LCIO'|g" sgv.steer
    sed -i -e "s|INPUT_FILENAMES = '\*.stdhep'|INPUT_FILENAMES = 'input.slcio'|g" sgv.steer
    sed -i -e "s|!   CALO_TREATMENT = 'PERF'|   CALO_TREATMENT = 'PERF'|g" sgv.steer
    sed -i -e "s|!  FILENAME = 'sgvout.slcio'|  FILENAME = 'sgvout.slcio'|g" sgv.steer
    sed -i -e "s|!  FULL_TRUTHLINK = .FALSE.|  FULL_TRUTHLINK = .TRUE.|g" sgv.steer

    # 2nd setup: copy from 1st and compile with samples/usercalo for ILD at 500 GeV
    # see $SGV_DIR/usercalo/samples/README for setup instructions
    cp -R "$SGV_DIR/tests" "$SGV_DIR/tests_beamcal_500GeV"
    cd "$SGV_DIR/tests_beamcal_500GeV"
    
    rm -f "$SGV_DIR/tests_beamcal_500GeV/sgv_geo3_ex.inp" "$SGV_DIR/tests_beamcal_500GeV/sgv_geo.inp"

    cp "$SGV_DIR/samples/usercalo/ztugeo-beamcal.F95" "$SGV_DIR/tests_beamcal_500GeV"
    ln -s "$SGV_DIR/samples/usercalo/bg_aver.sv01-14-01-p00.mILD_o1_v05.E500-TDR_ws.PBeamstr-pairs.I230000.SGVmap.txt" "$SGV_DIR/tests_beamcal_500GeV/fort.93"

    # set paths to this sub-directory
    rm -f "$SGV_DIR/tests_beamcal_500GeV/fort.17" "$SGV_DIR/tests_beamcal_500GeV/fort.51"
    ln -s "$SGV_DIR/tests_beamcal_500GeV/sgv.steer" "$SGV_DIR/tests_beamcal_500GeV/fort.17"
    ln -s "$SGV_DIR/tests_beamcal_500GeV/sgv_geo.inp" "$SGV_DIR/tests_beamcal_500GeV/fort.51"

    sed -i -e "s|!   PRIMARY_VERTEX_SIM = .FALSE.|    PRIMARY_VERTEX_SIM = .TRUE.|g" sgv.steer

    ln -s "$SGV_DIR/samples/usercalo/sgv_geo3_ex_bcal.inp" "$SGV_DIR/tests_beamcal_500GeV/sgv_geo.inp"
    
    gfortran -c -O3 -I$SGV_LIB/mod "$SGV_DIR/tests_beamcal_500GeV/ztugeo-beamcal.F95" -o "$SGV_DIR/tests_beamcal_500GeV/ztugeo-beamcal.o"
    cresgvexe merge usesgvlcio "-DEXTREAD" "" "" "" "eflow_par_type.o zaccon.o ztugeo-beamcal.o"

    # add LD_PRELOAD to sgvenv.sh to avoid missing libraries
    echo ". $SGV_DIR/samples/ld_preload_for_el9" >> "$SGV_DIR/sgvenv.sh"
}

sgv_install_action "$@"