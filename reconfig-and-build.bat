@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d C:\zenith\daw\build
echo Reconfiguring CMake with updated source list...
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
echo.
echo Building...
ninja ZenithDAW 2>&1 | findstr /I /C:"error" /C:"FAILED" /C:"Linking" /C:"Building CXX" | more +1
