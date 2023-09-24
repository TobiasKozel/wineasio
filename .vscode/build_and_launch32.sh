#!/bin/sh
SCRIPT_PATH=$(dirname $(realpath -s $0)) 
PREFIX_PATH=${SCRIPT_PATH}/prefix_debug

export WINEPREFIX=${PREFIX_PATH}

wineserver -k

make clean
make 32 DEBUG=true
screen -dm winedbg --gdb --no-start --port 11111 C:\\VBASIOTest32.exe
sleep 1