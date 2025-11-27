@echo off
echo ============================================
echo BUILDING ZENITH DAW NOW
echo ============================================
echo.

cd /d "%~dp0zenith-core\build"

if not exist "build.ninja" (
    echo ERROR: Build not configured!
    echo Please run: build-logic-pro.bat first
    pause
    exit /b 1
)

echo Compiling with Ninja...
echo This may take 5-10 minutes...
echo.

ninja -j 4

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo BUILD SUCCESSFUL!
    echo ============================================
    echo.
    echo Finding executable...
    for /r %%i in (*.exe) do (
        echo Found: %%i
    )
    echo.
    echo Look for ZenithDAW.exe above!
    echo.
) else (
    echo.
    echo BUILD FAILED!
    echo Check errors above.
    echo.
)

pause
