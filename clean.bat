@echo off
echo ==========================================
echo Zenith DAW - Clean Build Environment
echo ==========================================

echo Removing build directories...
if exist build rd /s /q build
if exist build2 rd /s /q build2
if exist build_test rd /s /q build_test
if exist out rd /s /q out

echo Removing build logs...
del /q build_log*.txt 2>nul
del /q build_output.txt 2>nul
del /q debug_log.txt 2>nul

echo Removing temp files...
del /q *.tmp 2>nul
del /q *.bak 2>nul

echo.
echo Clean complete. Ready for fresh build.
pause
