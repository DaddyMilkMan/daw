@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d C:\zenith\daw\build
echo ============================================
echo    FINAL BUILD - ALL FIXES COMPLETE  
echo ============================================
ninja ZenithDAW
