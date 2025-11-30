@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d C:\zenith\daw
rd /s /q build 2>nul
mkdir build
cd build
echo Configuring with fixed CMakeLists...
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=OFF 2>&1 | tail -n 50
echo.
echo Build configuration complete. Checking for build.ninja...
if exist build.ninja (
    echo SUCCESS: build.ninja generated\!
    echo Starting build...
    ninja ZenithDAW 2>&1 | tail -n 80
) else (
    echo FAILED: build.ninja not found
    exit /b 1
)
