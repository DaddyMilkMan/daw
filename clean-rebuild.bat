@echo off
echo Cleaning and reconfiguring build...
cd C:\zenith\daw
rd /s /q build 2>nul
mkdir build
cd build

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=OFF -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

echo.
echo CMake configuration complete. Now building...
ninja ZenithDAW 2>&1 | findstr /C:"error" /C:"Linking" /C:"built" | tail -n 15
