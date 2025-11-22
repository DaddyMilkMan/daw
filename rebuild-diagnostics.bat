@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d C:\zenith\daw\build
echo Rebuilding with accurate diagnostic logging...
ninja ZenithDAW 2>&1 | Select-Object -Last 15
