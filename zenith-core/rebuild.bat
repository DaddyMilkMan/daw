@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
echo Configuring...
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
if %ERRORLEVEL% NEQ 0 (
    echo Configuration failed!
    exit /b %ERRORLEVEL%
)
echo Building...
cmake --build build --config Debug --parallel 4
pause
