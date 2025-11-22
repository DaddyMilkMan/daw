@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d C:\zenith\daw\build
echo Building with SkiaRenderer integration...
ninja ZenithDAW 2>&1 | Select-String -Pattern 'Building|Linking|error|FAILED|SUCCESS' | Select-Object -Last 30
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo BUILD SUCCESSFUL\!
    echo ============================================
    if exist zenith-core\ZenithDAW.exe (
        echo Found: zenith-core\ZenithDAW.exe
    )
)
