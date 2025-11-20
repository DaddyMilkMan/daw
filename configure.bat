@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cd build
"C:\Program Files\CMake\bin\cmake.exe" -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
