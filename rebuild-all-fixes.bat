@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d C:\zenith\daw\build
echo Rebuilding after MainWindow and DebugLogOverlay fixes...
ninja ZenithDAW 2>&1 | tail -n 50
