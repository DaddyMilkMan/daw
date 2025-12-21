@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
echo Environment initialized.
where cl.exe
echo.
cd zenith-core
if exist build rmdir /s /q build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DZENITH_ENABLE_SKIA=OFF
if %ERRORLEVEL% NEQ 0 (
    echo CMake Failed.
    exit /b 1
)
echo CMake Succeeded.
cmake --build build --verbose
