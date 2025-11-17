@echo off
REM Run Script for Zenith DAW (Pure JUCE Native) - Windows

setlocal

set BUILD_TYPE=Release

REM Check if built
if not exist "zenith-core\build\ZenithDAW_artefacts\Release\ZenithDAW.exe" (
    if not exist "zenith-core\build\ZenithDAW_artefacts\Debug\ZenithDAW.exe" (
        echo Application not built yet. Building now...
        call build.bat Release
    ) else (
        set BUILD_TYPE=Debug
    )
)

REM Find executable
set EXECUTABLE=
if exist "zenith-core\build\ZenithDAW_artefacts\Release\ZenithDAW.exe" (
    set EXECUTABLE=zenith-core\build\ZenithDAW_artefacts\Release\ZenithDAW.exe
) else if exist "zenith-core\build\ZenithDAW_artefacts\Debug\ZenithDAW.exe" (
    set EXECUTABLE=zenith-core\build\ZenithDAW_artefacts\Debug\ZenithDAW.exe
)

if "%EXECUTABLE%"=="" (
    echo ERROR: ZenithDAW executable not found
    exit /b 1
)

echo =================================
echo Starting Zenith DAW (Native JUCE)
echo =================================
echo.

REM Run application
"%EXECUTABLE%" %*

endlocal
