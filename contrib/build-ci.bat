echo "== Build DRace ==="

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64

echo "=== Build drace-TSAN ==="
cd build-tsan
ninja
cd ..

echo "=== Build drace-Dummy ==="
cd build-dummy
ninja
cd ..
