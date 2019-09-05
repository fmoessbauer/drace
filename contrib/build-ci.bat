echo "== Build DRace ==="

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64

echo "=== Build drace-TSAN DR 7.91.x ==="
cd build-tsan-dr791
cmake --build .
cmake --build . --target install
cd ..

echo "=== Build drace-LEGACY-TSAN DR 7.91.x ==="
cd build-legacy-tsan-dr791
cmake --build .
cmake --build . --target install
cd ..

