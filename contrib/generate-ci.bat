echo "=== Setup build environment ==="

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 || goto :error

set CC=cl.exe
set CXX=cl.exe

echo "=== Generate drace-TSAN DR 7.91.x ==="
mkdir build-tsan-dr791
cd build-tsan-dr791
cmake -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDRACE_XML_EXPORTER=1 -DBOOST_ROOT=C:\\opt\\boost_1_71_0 -DCMAKE_PREFIX_PATH=C:\\Qt\\Qt5.13.1\\5.13.1\\msvc2017_64\\lib\\cmake\\Qt5 -DDRACE_ENABLE_TESTING=1 -DDRACE_ENABLE_BENCH=1 -DDynamoRIO_DIR=C:/opt/DynamoRIO-Windows-7.91.18081-0/cmake -DDRACE_DETECTOR=tsan -DCMAKE_INSTALL_PREFIX:PATH=package .. || goto :error
cd ..
goto :EOF

:error
echo At least one step failed with #%errorlevel%.
exit /b %errorlevel%

