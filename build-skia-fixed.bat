@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d C:\zenith\daw
echo Cleaning build directory...
rd /s /q build 2>nul
mkdir build
cd build
echo.
echo Configuring CMake with Skia (CPU rendering only)...
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
echo.
echo Building ZenithDAW with Skia...
ninja ZenithDAW 2>&1
