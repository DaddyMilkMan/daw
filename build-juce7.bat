@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d C:\zenith\daw
rd /s /q build 2>nul
mkdir build
cd build
echo Configuring with JUCE 7.0.12 and NMake...
cmake .. -G "NMake Makefiles" -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=OFF -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -n 60
echo.
if exist Makefile (
    echo SUCCESS: Makefile generated\!
    echo Starting build...
    nmake ZenithDAW 2>&1 | tail -n 100
) else (
    echo FAILED: Makefile not found
)
