echo "=== Setup build environment ==="

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64
set CC=cl.exe
set CXX=cl.exe

echo "=== Generate drace-TSAN DR 7.91.x ==="
mkdir build-tsan-dr791
cd build-tsan-dr791
cmake -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDRACE_XML_EXPORTER=1 -DDRACE_ENABLE_TESTING=1 -DDRACE_ENABLE_BENCH=1 -DDynamoRIO_DIR=C:/opt/DynamoRIO-Windows-7.91.18081-0/cmake -DDRACE_DETECTOR=tsan -DCMAKE_INSTALL_PREFIX:PATH=package ..
cd ..
