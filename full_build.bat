@echo off
setlocal enabledelayedexpansion

REM Set up Visual Studio 2022 environment
echo Setting up Visual Studio 2022 environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

if errorlevel 1 (
    echo Failed to set up Visual Studio environment
    exit /b 1
)

echo Environment set up successfully
where cl.exe
echo.

REM Clean and prepare build directory
cd /d C:\zenith\daw
echo Cleaning build directory...
rmdir /s /q build 2>nul
mkdir build
cd build

echo Running CMake with Ninja generator and MSVC...
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

if errorlevel 1 (
    echo CMake configuration failed
    exit /b 1
)

echo.
echo CMake configuration successful\!
echo Building with Ninja...
ninja ZenithDAW

echo.
echo Build exit code: \!ERRORLEVEL\!
