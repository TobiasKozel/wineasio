#!/bin/sh
SCRIPT_PATH=$(dirname $(realpath -s $0)) 
PREFIX_PATH=${SCRIPT_PATH}/prefix_debug

export WINEPREFIX=${PREFIX_PATH}

wineserver -k
exit 0