@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d C:\zenith\daw\build
echo Final build...
ninja ZenithDAW
echo.
echo Done\! Exit code: %ERRORLEVEL%
