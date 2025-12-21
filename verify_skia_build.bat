@echo off
call C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat x64 >nul 2>&1
cd /d C:\zenith\daw
echo Cleaning build directory...
rmdir /s /q build 2>nul
mkdir build
cd build
echo Configuring with CMake (Skia enabled)...
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
echo.
echo Building ZenithDAW...
ninja ZenithDAW
echo.
echo Build complete\!
if exist zenith-core\ZenithDAW.exe (
    echo SUCCESS: ZenithDAW.exe found\!
    dir zenith-core\ZenithDAW.exe
) else (
    echo Checking for executable...
    dir /s /b *.exe 2>nul | findstr /i zenith
)
