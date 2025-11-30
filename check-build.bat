@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d C:\zenith\daw\build
echo Building with all fixes...
ninja ZenithDAW 2>&1 | findstr /C:"error" /C:"error:" /C:"FAILED" /C:"Linking" | tail -n 40
