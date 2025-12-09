@echo off
setlocal enabledelayedexpansion

echo Waiting for Zenith DAW with Skia to be built...
timeout /t 2 /nobreak >nul

:wait_loop
if not exist "C:\zenith\daw\build\zenith-core\ZenithDAW_artefacts\Release\Zenith DAW.exe" (
    echo Still building... 
    timeout /t 10 /nobreak >nul
    goto wait_loop
)

echo.
echo ============================================
echo LAUNCHING ZENITH DAW WITH SKIA!
echo ============================================
echo.

start "" "C:\zenith\daw\build\zenith-core\ZenithDAW_artefacts\Release\Zenith DAW.exe"

echo.
echo Zenith DAW launched!
echo ============================================
