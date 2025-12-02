@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd test_cmake
mkdir build
cd build
cmake .. -G Ninja
ninja
