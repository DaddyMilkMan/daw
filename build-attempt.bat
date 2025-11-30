@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d C:\zenith\daw
rd /s /q build 2>nul
mkdir build
cd build
echo Configuring...
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=OFF
echo Checking if build was successful...
if exist build.ninja (
    echo build.ninja exists, proceeding with build...
    ninja ZenithDAW
) else (
    echo Configuration failed, build.ninja not found
    exit /b 1
)
