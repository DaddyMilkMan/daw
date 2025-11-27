@echo off
echo ============================================
echo COMPLETING BUILD
echo ============================================
echo.

cd /d "%~dp0zenith-core\build"

if not exist "CMakeCache.txt" (
    echo Build not configured yet!
    pause
    exit /b 1
)

echo Running ninja to compile...
ninja -v

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo SUCCESS!
    echo ============================================
    dir /s /b *.exe 2>nul | findstr /i "Zenith"
) else (
    echo Build failed!
)

pause
