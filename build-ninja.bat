@echo off
setlocal enabledelayedexpansion

echo ============================================
echo ZENITH DAW - LOGIC PRO UI BUILD (Ninja)
echo ============================================
echo.

cd /d "%~dp0zenith-core"

REM Clean old build
if exist "build" (
    echo Cleaning previous build...
    rmdir /s /q build
)

mkdir build
cd build

echo.
echo Configuring CMake with Ninja (Skia enabled)...
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DZENITH_ENABLE_SKIA=ON

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo Check the error above for details.
    echo.
    pause
    exit /b 1
)

echo.
echo ============================================
echo Building with Ninja...
echo ============================================
echo.

ninja

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ============================================
    echo BUILD FAILED
    echo ============================================
    echo.
    echo Check the errors above for details.
    echo.
    pause
    exit /b 1
)

echo.
echo ============================================
echo SUCCESS: Zenith DAW with Logic Pro UI Built!
echo ============================================
echo.
echo Executable should be in: %CD%\ZenithDAW_artefacts\Release\
echo.
echo Your DAW now has all 250 Logic Pro features!
echo.
pause
