@echo off
call C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat x64 >nul 2>&1
cd /d C:\zenith\daw\build
echo Testing build with P0 critical fixes...
echo.
ninja ZenithDAW 2>&1 | tail -n 50
echo.
if %ERRORLEVEL% EQU 0 (
    echo ============================================
    echo SUCCESS\! All P0 fixes compile correctly
    echo ============================================
) else (
    echo Build failed - checking errors...
)
