@echo off
setlocal enabledelayedexpansion

REM ============================================================================
REM Zenith DAW - Direct Skia Rendering Build Script
REM ============================================================================

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

cd /d C:\zenith\daw

echo.
echo ============================================================================
echo Building Zenith DAW with Direct Skia Rendering
echo ============================================================================
echo.

REM Clean and rebuild
echo [1/3] Cleaning build directory...
rmdir /s /q build 2>nul
mkdir build
cd build

echo [2/3] Running CMake configuration...
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed!
    exit /b 1
)

echo [3/3] Building ZenithDAW...
ninja ZenithDAW 2>&1 | tail -100

if errorlevel 1 (
    echo.
    echo ============================================================================
    echo BUILD FAILED - Check errors above
    echo ============================================================================
    exit /b 1
)

echo.
echo ============================================================================
echo BUILD SUCCESSFUL!
echo ============================================================================
echo.
echo Executable location:
if exist zenith-core\ZenithDAW.exe (
    echo   ✓ build\zenith-core\ZenithDAW.exe
    dir zenith-core\ZenithDAW.exe | findstr /C:"ZenithDAW.exe"
) else (
    echo Searching for executable...
    for /r . %%F in (ZenithDAW.exe) do echo   ✓ %%F
)

echo.
echo To run: cd build ^&^& zenith-core\ZenithDAW.exe
echo.
