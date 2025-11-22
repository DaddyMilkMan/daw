@echo off
setlocal enabledelayedexpansion

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

cd /d C:\zenith\daw\build
echo Rebuilding after all fixes...
ninja ZenithDAW 2>&1 | tail -100
