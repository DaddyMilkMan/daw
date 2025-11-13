@echo off
REM Zenith DAW - Native JUCE Build Script

setlocal enabledelayedexpansion

if "%1"=="" (
    set BUILD_TYPE=Release
) else (
    set BUILD_TYPE=%1
)

set BUILD_DIR=build

echo.
echo =================================
echo Zenith DAW Native Build
echo =================================
echo Build type: %BUILD_TYPE%
echo.

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cmake -S . -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 goto :fail

cmake --build "%BUILD_DIR%" --config %BUILD_TYPE%
if errorlevel 1 goto :fail

echo.
echo Build artifacts are in %BUILD_DIR%
echo.
goto :eof

:fail
echo Build failed.
exit /b 1

:eof
endlocal
