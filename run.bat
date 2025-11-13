@echo off
REM Launch Zenith DAW native build. Builds automatically if needed.

setlocal enabledelayedexpansion

if "%1"=="" (
    set CONFIG=Release
) else (
    set CONFIG=%1
)

set BUILD_DIR=build

if not exist "%BUILD_DIR%" (
    echo Build directory missing. Building now...
    call build.bat %CONFIG%
)

set APP_PATH=%BUILD_DIR%\zenith-core\ZenithDAW_artefacts\%CONFIG%\ZenithDAW.exe
if not exist "%APP_PATH%" (
    call build.bat %CONFIG%
)

if not exist "%APP_PATH%" (
    echo Unable to find ZenithDAW executable after rebuild.
    exit /b 1
)

"%APP_PATH%"

endlocal
