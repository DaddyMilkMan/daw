@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d C:\zenith\daw\build
echo Building ZenithDAW with all fixes...
ninja ZenithDAW 2>&1 | tail -n 80
