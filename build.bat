@echo off
REM ========================================
REM ZENITH DAW - Master Build Script
REM Consolidated by Sophia "The Architect" Chen
REM ========================================

setlocal enabledelayedexpansion

echo.
echo ╔════════════════════════════════════════╗
echo ║      ZENITH DAW - MASTER BUILD         ║
echo ║   Sophia "The Architect" Chen          ║
echo ╚════════════════════════════════════════╝
echo.

REM Parse command line arguments
set "BUILD_TYPE=Release"
set "ENABLE_SKIA=ON"
set "CLEAN_BUILD=0"

:parse_args
if "%~1"=="" goto end_parse
if /i "%~1"=="--debug" set "BUILD_TYPE=Debug"
if /i "%~1"=="--release" set "BUILD_TYPE=Release"
if /i "%~1"=="--no-skia" set "ENABLE_SKIA=OFF"
if /i "%~1"=="--clean" set "CLEAN_BUILD=1"
if /i "%~1"=="--help" goto show_help
shift
goto parse_args
:end_parse

echo Configuration:
echo   Build Type: %BUILD_TYPE%
echo   Skia: %ENABLE_SKIA%
echo   Clean Build: %CLEAN_BUILD%
echo.

REM Clean if requested
if "%CLEAN_BUILD%"=="1" (
    echo [1/4] Cleaning previous build...
    if exist zenith-core\build rmdir /S /Q zenith-core\build
    echo ✓ Clean complete
    echo.
)

REM Configure CMake
echo [2/4] Configuring CMake...
cd zenith-core
if not exist build mkdir build
cd build

cmake .. ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DZENITH_USE_SKIA=%ENABLE_SKIA% ^
    -DCMAKE_PREFIX_PATH="C:/vcpkg/installed/x64-windows"

if errorlevel 1 (
    echo ✗ CMake configuration failed!
    cd ..\..
    pause
    exit /b 1
)
echo ✓ Configuration complete
echo.

REM Build
echo [3/4] Building Zenith DAW...
cmake --build . --config %BUILD_TYPE% --parallel

if errorlevel 1 (
    echo ✗ Build failed!
    cd ..\..
    pause
    exit /b 1
)
echo ✓ Build complete
echo.

REM Copy artifacts to root for easy access
echo [4/4] Copying executable...
copy /Y "ZenithDAW_artefacts\%BUILD_TYPE%\Zenith DAW.exe" "..\..\Zenith DAW.exe" >nul
if exist "ZenithDAW_artefacts\%BUILD_TYPE%\*.dll" (
    copy /Y "ZenithDAW_artefacts\%BUILD_TYPE%\*.dll" "..\..\" >nul
)

cd ..\..

echo.
echo ╔════════════════════════════════════════╗
echo ║         BUILD SUCCESSFUL! ✓            ║
echo ╚════════════════════════════════════════╝
echo.
echo Executable: Zenith DAW.exe
echo Type: %BUILD_TYPE%
echo.
echo Run: "Zenith DAW.exe" to start
echo.
pause
exit /b 0

:show_help
echo.
echo ZENITH DAW Master Build Script
echo.
echo Usage: build.bat [options]
echo.
echo Options:
echo   --debug       Build in Debug mode (default: Release)
echo   --release     Build in Release mode
echo   --no-skia     Build without Skia rendering
echo   --clean       Clean build directory before building
echo   --help        Show this help message
echo.
echo Examples:
echo   build.bat                  (Release build with Skia)
echo   build.bat --debug          (Debug build)
echo   build.bat --clean          (Clean + rebuild)
echo   build.bat --no-skia        (Build without Skia)
echo.
pause
exit /b 0
