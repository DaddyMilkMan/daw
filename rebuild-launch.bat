@echo off
cd /d C:\zenith\daw\build
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
ninja ZenithDAW
if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    start "" "C:\zenith\daw\build\zenith-core\ZenithDAW_artefacts\Debug\Zenith DAW.exe"
) else (
    echo Build failed!
)
