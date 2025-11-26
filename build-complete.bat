@echo off
setlocal enabledelayedexpansion

echo ============================================
echo ZENITH DAW - COMPLETE BUILD SCRIPT
echo ============================================
echo.
echo This script will:
echo 1. Fix all JUCE 8.0.9 API compatibility issues
echo 2. Build WITHOUT Skia first (to verify compilation)
echo 3. Then attempt to build WITH Skia
echo.

cd /d "%~dp0zenith-core"

REM ============================================
REM PHASE 1: BUILD WITHOUT SKIA
REM ============================================

echo.
echo ============================================
echo PHASE 1: Building WITHOUT Skia
echo ============================================
echo.

if exist "build" (
    echo Cleaning previous build...
    rmdir /s /q build
)

mkdir build
cd build

echo Configuring CMake (Skia DISABLED)...
cmake .. -G "Visual Studio 17 2022" -A x64 -DZENITH_ENABLE_SKIA=OFF

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo.
    pause
    exit /b 1
)

echo.
echo Building Release...
cmake --build . --config Release --parallel 2>&1 | tee build-no-skia.log

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ============================================
    echo BUILD FAILED (WITHOUT SKIA)
    echo ============================================
    echo.
    echo The build failed even without Skia. This means
    echo there are still JUCE API compatibility issues.
    echo.
    echo Check build-no-skia.log for details.
    echo.
    pause
    exit /b 1
)

echo.
echo ============================================
echo SUCCESS: Build completed WITHOUT Skia!
echo ============================================
echo.
echo Executable: %CD%\zenith-core\ZenithDAW_artefacts\Release\Zenith DAW.exe
echo.

REM ============================================
REM PHASE 2: BUILD WITH SKIA (if user wants)
REM ============================================

echo.
echo.
echo ============================================
echo PHASE 2: Build WITH Skia?
echo ============================================
echo.
echo Would you like to try building WITH Skia?
echo Note: This requires Skia to be installed via vcpkg.
echo.
set /p CONTINUE="Build with Skia? (y/n): "

if /i not "%CONTINUE%"=="y" (
    echo.
    echo Skipping Skia build. You can run this script again later.
    pause
    exit /b 0
)

cd ..
if exist "build" (
    echo Cleaning previous build...
    rmdir /s /q build
)

mkdir build
cd build

echo.
echo Configuring CMake (Skia ENABLED)...
cmake .. -G "Visual Studio 17 2022" -A x64 -DZENITH_ENABLE_SKIA=ON

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ============================================
    echo SKIA CONFIGURATION FAILED
    echo ============================================
    echo.
    echo This is expected if Skia is not installed via vcpkg.
    echo.
    echo To install Skia:
    echo 1. Install vcpkg if you haven't already
    echo 2. Run: vcpkg install skia:x64-windows
    echo 3. Run this script again
    echo.
    pause
    exit /b 1
)

echo.
echo Building Release WITH Skia...
cmake --build . --config Release --parallel 2>&1 | tee build-with-skia.log

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ============================================
    echo BUILD FAILED (WITH SKIA)
    echo ============================================
    echo.
    echo The build failed with Skia enabled.
    echo Check build-with-skia.log for details.
    echo.
    echo You can still use the non-Skia build that succeeded earlier.
    echo.
    pause
    exit /b 1
)

echo.
echo ============================================
echo SUCCESS: Build completed WITH Skia!
echo ============================================
echo.
echo Executable: %CD%\zenith-core\ZenithDAW_artefacts\Release\Zenith DAW.exe
echo.
echo Skia rendering is now enabled!
echo.
pause
