echo "=== Setup build environment ==="

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64

echo "=== Generate drace-TSAN ==="
mkdir build-tsan
cd build-tsan
cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDRACE_XML_EXPORTER=1 -DDRACE_ENABLE_TESTING=1 -DDRACE_ENABLE_BENCH=1 -DDynamoRIO_DIR=C:/opt/DynamoRIO-Windows-7.0.17837-0/cmake -DDRACE_DETECTOR=tsan ..
cd ..

echo "=== Generate drace-Dummy ==="
mkdir build-dummy
cd build-dummy
cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDRACE_XML_EXPORTER=1 -DDRACE_ENABLE_TESTING=1 -DDRACE_ENABLE_BENCH=1 -DDynamoRIO_DIR=C:/opt/DynamoRIO-Windows-7.0.17837-0/cmake -DDRACE_DETECTOR=dummy ..
cd ..
