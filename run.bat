@echo off
REM Run Script for Vexel DAW Qt/QML Application (Windows)

setlocal

REM Check if built
if not exist "build\bin\Release\VexelDAW.exe" (
    if not exist "build\bin\Debug\VexelDAW.exe" (
        echo Application not built yet. Building now...
        call build.bat Release
    )
)

REM Find executable
set EXECUTABLE=
if exist "build\bin\Release\VexelDAW.exe" (
    set EXECUTABLE=build\bin\Release\VexelDAW.exe
) else if exist "build\bin\Debug\VexelDAW.exe" (
    set EXECUTABLE=build\bin\Debug\VexelDAW.exe
)

if "%EXECUTABLE%"=="" (
    echo ERROR: VexelDAW executable not found
    exit /b 1
)

echo =================================
echo Starting Vexel DAW...
echo =================================
echo.

REM Run application
"%EXECUTABLE%" %*

endlocal
