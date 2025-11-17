@echo off
REM Build Script for Zenith DAW (Pure JUCE Native) - Windows
REM
REM This script builds the native JUCE DAW application
REM Usage: build.bat [Debug|Release]

setlocal

REM Configuration
if "%1"=="" (
    set BUILD_TYPE=Release
) else (
    set BUILD_TYPE=%1
)

set BUILD_DIR=zenith-core\build

echo =================================
echo Zenith DAW - Pure JUCE Native Build
echo =================================
echo Build type: %BUILD_TYPE%
echo.

REM Check for CMake
echo Checking for CMake...
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found!
    echo Install from: https://cmake.org/download/
    exit /b 1
)

for /f "tokens=*" %%i in ('cmake --version') do (
    echo Found %%i
    goto :cmake_found
)
:cmake_found
echo.

REM Create build directory
echo Creating build directory...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

REM Configure with CMake
echo Configuring CMake...
echo   - Fetching JUCE 8.0.9 (if needed)...
cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -G "Visual Studio 17 2022"

REM Build
echo.
echo Building Zenith DAW...
cmake --build . --config %BUILD_TYPE% -j

REM Success
echo.
echo =================================
echo Build completed successfully!
echo =================================
echo.
echo Architecture: Pure C++ JUCE Native
echo   - No Electron
echo   - No React/TypeScript
echo   - No Qt/QML
echo   - Pure JUCE GUI
echo.
echo To run the application:
echo   zenith-core\build\ZenithDAW_artefacts\%BUILD_TYPE%\ZenithDAW.exe
echo.
echo Or use: run.bat
echo.

cd ..\..
endlocal
