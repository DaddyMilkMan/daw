@echo off
call C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat x64 >nul 2>&1
cd /d C:\zenith\daw\build
echo Building ZenithDAW with all Skia fixes applied...
ninja ZenithDAW
echo.
echo Build complete\! Exit code: %ERRORLEVEL%
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo SUCCESS\! Build completed without errors
    echo ============================================
    echo.
    echo Checking for executable...
    if exist zenith-core\ZenithDAW.exe (
        dir zenith-core\ZenithDAW.exe
    )
)
