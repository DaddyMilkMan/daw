@echo off
call C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat x64 >nul 2>&1
cd /d C:\zenith\daw\build
echo Building with Grok 4.1 integration...
ninja ZenithDAW
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo BUILD SUCCESSFUL - Grok 4.1 Integration Ready\!
    echo ============================================
    echo.
    dir zenith-core\ZenithDAW.exe
)
