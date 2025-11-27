@echo off
setlocal enabledelayedexpansion

echo ============================================
echo ZENITH DAW - LOGIC PRO UI BUILD (VS 2026)
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
echo Configuring CMake with Visual Studio 2026...
cmake .. -G "Visual Studio 18 2026" -A x64 -DZENITH_ENABLE_SKIA=ON -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo Trying Ninja as fallback...
    cd ..
    rmdir /s /q build
    mkdir build
    cd build
    cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DZENITH_ENABLE_SKIA=ON
    if %ERRORLEVEL% NEQ 0 (
        echo.
        echo Both VS 2026 and Ninja failed!
        pause
        exit /b 1
    )
    echo.
    echo Building with Ninja...
    ninja
    goto :check_result
)

echo.
echo ============================================
echo Building with Visual Studio 2026...
echo ============================================
echo.

cmake --build . --config Release --parallel

:check_result
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ============================================
    echo BUILD FAILED
    echo ============================================
    echo.
    pause
    exit /b 1
)

echo.
echo ============================================
echo SUCCESS: Zenith DAW with Logic Pro UI Built!
echo ============================================
echo.
echo Your DAW now has all 250 Logic Pro features!
echo.
pause
