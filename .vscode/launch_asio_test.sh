#!/bin/sh
SCRIPT_PATH=$(dirname $(realpath -s $0)) 
PREFIX_PATH=${SCRIPT_PATH}/prefix_debug

export WINEPREFIX=${PREFIX_PATH}

wine64 C:\\VBASIOTest64.exe