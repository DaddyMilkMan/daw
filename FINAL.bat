@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d C:\zenith\daw\build
echo === FINAL COMPILATION ===
ninja ZenithDAW
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo BUILD SUCCESSFUL\! 
    echo ============================================
    echo.
    dir zenith-core\ZenithDAW.exe
) else (
    echo BUILD FAILED with code %ERRORLEVEL%
)
