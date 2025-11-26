@echo off
echo ============================================
echo ZENITH DAW - COMPLETE SKIA SETUP
echo ============================================
echo.
echo This script will:
echo 1. Download pre-built Skia binaries
echo 2. Update CMakeLists.txt for manual Skia
echo 3. Build Zenith DAW with Skia enabled
echo.
echo This will take 5-10 minutes.
echo.
pause

cd /d "%~dp0"

REM ============================================
REM STEP 1: Download Skia
REM ============================================

echo.
echo ============================================
echo STEP 1/3: Downloading Skia...
echo ============================================
echo.

call download-skia.bat

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Failed to download Skia
    pause
    exit /b 1
)

REM ============================================
REM STEP 2: Update CMakeLists
REM ============================================

echo.
echo ============================================
echo STEP 2/3: Updating CMakeLists.txt...
echo ============================================
echo.

call update-cmake-for-skia.bat

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Failed to update CMakeLists.txt
    pause
    exit /b 1
)

REM ============================================
REM STEP 3: Build with Skia
REM ============================================

echo.
echo ============================================
echo STEP 3/3: Building with Skia...
echo ============================================
echo.

call build-with-manual-skia.bat

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ============================================
echo SUCCESS! SKIA IS NOW RUNNING!
echo ============================================
echo.
echo Your Zenith DAW now has GPU-accelerated Skia rendering!
echo.
echo Run the executable:
echo C:\zenith\daw\zenith-core\build\zenith-core\ZenithDAW_artefacts\Release\Zenith DAW.exe
echo.
pause
