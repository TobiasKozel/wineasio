#!/bin/sh
SCRIPT_PATH=$(dirname $(realpath -s $0)) 
PREFIX_PATH=${SCRIPT_PATH}/prefix_debug

export WINEPREFIX=${PREFIX_PATH}

wineserver -k

make clean
make 64 DEBUG=true
screen -dm winedbg --gdb --no-start --port 11111 C:\\VBASIOTest64.exe
sleep 1