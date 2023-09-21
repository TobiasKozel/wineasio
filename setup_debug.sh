#!/bin/sh
# Sets up a wine prefix for debugging and downloads the VB Audio Asio Tester

# path to self, hard code this if it's causing troubles
SCRIPT_PATH=$(dirname $(realpath -s $0)) 

PREFIX_PATH=${SCRIPT_PATH}/prefix_debug
BUILD_PATH=${SCRIPT_PATH}/build64

export WINEPREFIX=${PREFIX_PATH}

wineserver -k

rm -rf ${PREFIX_PATH}

echo "Setting up wineprefix at ${PREFIX_PATH}"
wineboot -i

make 64 DEBUG=true
# make 32 DEBUG=true

# sudo cp build64/wineasio.dll /usr/lib/wine/x86_64-windows/wineasio.dll
# sudo cp build64/wineasio.dll.so /usr/lib/wine/x86_64-unix/wineasio.dll.so

# sudo ln -s build64/wineasio.dll /usr/lib/wine/x86_64-windows/
# sudo ln -s build64/wineasio.dll.so /usr/lib/wine/x86_64-unix/

# sudo ln -s build64/wineasio.dll /usr/lib/wine/x86_64-windows/
# sudo ln -s build64/wineasio.dll.so /usr/lib/wine/x86_64-unix/

# LD_LIBRARY_PATH=${BUILD_PATH}
wine64 regsvr32 wineasio.dll


# cp build64/wineasio.dll ./prefix_debug/drive_c/windows/system32/

cd ${PREFIX_PATH}/drive_c/
wget https://download.vb-audio.com/Download_MT128/VBAsioTest_1014.zip
unzip VBAsioTest_1014.zip

# LD_LIBRARY_PATH=${BUILD_PATH} 
wine64 ${PREFIX_PATH}/drive_c/VBASIOTest64.exe


find . | grep wineasio.dll