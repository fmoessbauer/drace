echo "== Build DRace ==="

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64

echo "=== Build drace-TSAN DR 7.0.x ==="
cd build-tsan-dr70
cmake --build .
cd ..

echo "=== Build drace-TSAN DR 7.1.x ==="
cd build-tsan-dr71
cmake --build .
cd ..

echo "=== Build drace-Dummy ==="
cd build-dummy
cmake --build .
cd ..

