@echo off
REM Run Script for Zenith DAW Qt/QML Application (Windows)

setlocal

REM Check if built
if not exist "build\bin\Release\ZenithDAW.exe" (
    if not exist "build\bin\Debug\ZenithDAW.exe" (
        echo Application not built yet. Building now...
        call build.bat Release
    )
)

REM Find executable
set EXECUTABLE=
if exist "build\bin\Release\ZenithDAW.exe" (
    set EXECUTABLE=build\bin\Release\ZenithDAW.exe
) else if exist "build\bin\Debug\ZenithDAW.exe" (
    set EXECUTABLE=build\bin\Debug\ZenithDAW.exe
)

if "%EXECUTABLE%"=="" (
    echo ERROR: ZenithDAW executable not found
    exit /b 1
)

echo =================================
echo Starting Zenith DAW...
echo =================================
echo.

REM Run application
"%EXECUTABLE%" %*

endlocal
