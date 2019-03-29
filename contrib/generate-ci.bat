echo "=== Setup build environment ==="

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64
set CC=cl.exe
set CXX=cl.exe

echo "=== Generate drace-TSAN DR 7.0.x ==="
mkdir build-tsan-dr70
cd build-tsan-dr70
cmake -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDRACE_XML_EXPORTER=1 -DDRACE_ENABLE_TESTING=1 -DDRACE_ENABLE_BENCH=1 -DDynamoRIO_DIR=C:/opt/DynamoRIO-Windows-7.0.17837-0/cmake -DDRACE_DETECTOR=tsan ..
cd ..

echo "=== Generate drace-TSAN DR 7.1.x ==="
mkdir build-tsan-dr71
cd build-tsan-dr71
cmake -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDRACE_XML_EXPORTER=1 -DDRACE_ENABLE_TESTING=1 -DDRACE_ENABLE_BENCH=1 -DDynamoRIO_DIR=C:/opt/DynamoRIO-Windows-7.1.17963-0/cmake -DDRACE_DETECTOR=tsan ..
cd ..

echo "=== Generate drace-Dummy ==="
mkdir build-dummy
cd build-dummy
cmake -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDRACE_XML_EXPORTER=1 -DDRACE_ENABLE_TESTING=1 -DDRACE_ENABLE_BENCH=1 -DDynamoRIO_DIR=C:/opt/DynamoRIO-Windows-7.0.17837-0/cmake -DDRACE_DETECTOR=dummy ..
cd ..

