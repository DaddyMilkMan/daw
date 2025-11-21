@echo off
setlocal enabledelayedexpansion

REM Set up Visual Studio 2026 environment
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

REM Clean and rebuild
cd /d C:\zenith\daw
rmdir /s /q build 2>nul
mkdir build
cd build

echo Configuring with CMake...
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

if errorlevel 1 (
    echo CMake configuration failed
    exit /b 1
)

echo.
echo Building with Ninja...
ninja ZenithDAW 2>&1

echo.
echo Build complete\!
echo.
echo Checking for executable...
if exist zenith-core\ZenithDAW.exe (
    echo.
    echo ^^^^^^^^ SUCCESS\! ^^^^^^^^
    echo Found ZenithDAW.exe
    dir zenith-core\ZenithDAW.exe
) else (
    echo.
    echo Searching for any .exe files...
    for /r . %%%%F in (*.exe) do echo Found: %%%%F
)
