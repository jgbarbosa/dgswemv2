#!/bin/bash

START_BOLD="tput bold"
END_BOLD="tput sgr0"

usage () {
    echo "usage: $0 [options]"
    echo "options:"
    echo "    -h"
    echo "        shows this message"
    echo "    -c <config_file> "
    echo "        specify a config file."
    echo "        default config file is config.txt"
    echo "    clean"
    echo "        removes build directory."
    echo "    no-make"
    echo "        runs CMake, does not run make."
    echo "    reinstall"
    echo "        if already built, reinstalls into install dir."
    exit 1
}

CONFIGFILE=""

if [ "$#" -gt 2 ]; then usage; fi

if [ "$#" == 2 ] && [ "$1" == "-c" ]; then
    CONFIGFILE=$2
fi

if [ "$1" == "-h" ]; then usage; fi
if [ "$1" == "--help" ]; then usage; fi

# Fix me: handle more options
if [ "$#" -gt 0 ]; then
    if [ "$1" != "clean" ] && [ "$1" != "no-make" ] && [ "$1" != "reinstall"]; then
	echo "invalid option: $1"
	usage
    fi
fi

SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
source $SCRIPTPATH/util.sh

if [ -z "$CONFIGFILE" ]; then
    CONFIGFILE=${SCRIPTPATH}/config.txt
fi

if [ -z "$CONFIGFILE" ]; then
    echo "CONFIGFILE variable not set, exiting!"
    exit 1
fi

load_config_file $CONFIGFILE

HWLOC_BUILD_PATH="${BUILD_PATH}/hwloc"

if [ "$1" == "clean" ]; then
    clean_up $HWLOC_BUILD_PATH
fi

try_loading_modules $MODULES

if [ ! -d "$HWLOC_BUILD_PATH" ]; then
    set -e
    mkdir -p ${HWLOC_BUILD_PATH}
    cd ${HWLOC_BUILD_PATH}
    wget https://www.open-mpi.org/software/hwloc/v1.11/downloads/hwloc-1.11.7.tar.gz
    tar xf hwloc-1.11.7.tar.gz
    cd hwloc-1.11.7
    ./configure --prefix=${INSTALL_PATH}
    make
    make install
else
    $START_BOLD
    echo "directory exists! please either run:"
    $END_BOLD
    echo "$0 clean"
    $START_BOLD
    echo "or continue the build process:"
    $END_BOLD
    echo "cd ${HWLOC_BUILD_PATH}"
    echo "make"
    echo "make install"
    exit 0
fi
