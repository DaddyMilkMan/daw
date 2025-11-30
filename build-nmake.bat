@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d C:\zenith\daw
rd /s /q build 2>nul
mkdir build
cd build
echo Configuring with NMake Makefiles...
cmake .. -G "NMake Makefiles" -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=OFF -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -n 60
