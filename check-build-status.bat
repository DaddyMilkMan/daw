@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d C:\zenith\daw\build
echo Checking last build status and attempting rebuild...
ninja ZenithDAW 2>&1 | tail -n 100
