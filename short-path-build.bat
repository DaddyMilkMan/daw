@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d C:if not exist C:\zb mkdir C:\zb
cd C:\zb
if exist build rd /s /q build
mkdir build
cd build
echo Building from short path C:\zb\build...
cmake C:\zenith\daw -G Ninja -DZENITH_ENABLE_SKIA=OFF -DCMAKE_BUILD_TYPE=Release
