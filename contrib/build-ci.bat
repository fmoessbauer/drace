echo "== Build DRace ==="

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 || ^
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64

echo "=== Build drace-TSAN DR 7.91.x ==="
cd build-tsan-dr791
cmake --build . || goto :error
cmake --build . --target install || goto :error
cd ..
goto :EOF

:error
echo At least one step failed with #%errorlevel%.
exit /b %errorlevel%

