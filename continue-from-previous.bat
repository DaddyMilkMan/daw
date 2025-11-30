@echo off
call C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat x64 >nul 2>&1
cd /d C:\zenith\daw\build
echo Checking current build status and continuing...
ninja ZenithDAW 2>&1 | Select-Object -Last 50
