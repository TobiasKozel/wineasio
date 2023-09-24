#!/bin/sh
# Sets up a wine prefix for debugging and downloads the VB Audio Asio Tester

# path to self, hard code this if it's causing troubles
SCRIPT_PATH=$(dirname $(realpath -s $0)) 

PREFIX_PATH=${SCRIPT_PATH}/prefix_debug
BUILD_PATH=${SCRIPT_PATH}/../build64

export WINEPREFIX=${PREFIX_PATH}
export LD_LIBRARY_PATH=${BUILD_PATH} 

wineserver -k

rm -rf ${PREFIX_PATH}

wineboot -i

make DEBUG=true


# doesn't work
# cp build64/wineasio.dll ${PREFIX_PATH}/drive_c/windows/syswow64/
# cp build64/wineasio.dll.so ${PREFIX_PATH}/drive_c/windows/syswow64/

# final installation
# sudo cp build64/wineasio.dll /usr/lib/wine/x86_64-windows/wineasiopw.dll
# sudo cp build64/wineasio.dll.so /usr/lib/wine/x86_64-unix/wineasiopw.dll.so

# debugging
# sudo ln -s ${BUILD_PATH}/wineasiopw.dll /usr/lib/wine/x86_64-windows/
# sudo ln -s ${BUILD_PATH}/wineasiopw.dll.so /usr/lib/wine/x86_64-unix/

# sudo rm /usr/lib/wine/x86_64-windows/wineasiopw.dll
# sudo rm /usr/lib/wine/x86_64-unix/wineasiopw.dll.so


wine64 regsvr32 wineasiopw.dll

cd ${PREFIX_PATH}/drive_c/
wget https://download.vb-audio.com/Download_MT128/VBAsioTest_1014.zip
unzip VBAsioTest_1014.zip
