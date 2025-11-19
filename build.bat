@echo off
REM Build Script for Zenith DAW Qt/QML Application (Windows)
REM
REM This script builds the Qt/QML + JUCE hybrid DAW application
REM Usage: build.bat [Debug|Release]

setlocal

REM Configuration
if "%1"=="" (
    set BUILD_TYPE=Release
) else (
    set BUILD_TYPE=%1
)

set BUILD_DIR=build

echo =================================
echo Zenith DAW - Qt/QML + JUCE Build
echo =================================
echo Build type: %BUILD_TYPE%
echo.

REM Check for Qt
echo Checking for Qt...
where qmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Qt not found!
    echo.
    echo Please install Qt 6.5+ from: https://www.qt.io/download-qt-installer
    echo.
    echo Or add Qt to PATH:
    echo   set PATH=C:\Qt\6.x\msvc2022_64\bin;%%PATH%%
    exit /b 1
)

for /f "tokens=*" %%i in ('qmake -query QT_VERSION') do set QT_VERSION=%%i
echo Found Qt version: %QT_VERSION%
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

REM Check for JUCE
if exist "JUCE\" (
    echo Found JUCE framework
) else (
    echo WARNING: JUCE not found - using placeholder implementation
    echo    To add JUCE: git submodule add https://github.com/juce-framework/JUCE.git
)
echo.

REM Create build directory
echo Creating build directory...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

REM Configure with CMake
echo Configuring CMake...
cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -G "Visual Studio 17 2022"

REM Build
echo.
echo Building...
cmake --build . --config %BUILD_TYPE% -j

REM Success
echo.
echo =================================
echo Build completed successfully!
echo =================================
echo.
echo To run the application:
echo   build\bin\%BUILD_TYPE%\ZenithDAW.exe
echo.
echo Or use: run.bat
echo.

cd ..
endlocal
