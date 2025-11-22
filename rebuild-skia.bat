@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d C:\zenith\daw\build
echo Rebuilding with Skia enabled...
ninja ZenithDAW 2>&1 | Select-String -Pattern 'error|FAILED|Linking|Building CXX' | Select-Object -Last 50
