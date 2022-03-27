#!/bin/sh
wineserver -k
make clean
make DEBUG=true 64
PIPEWIRE_LATENCY=256/48000 screen -dm winedbg --gdb --no-start --port 13337 ${HOME}/.wine/drive_c/VBASIOTest64.exe
sleep 1