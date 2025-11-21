@echo off
setlocal enabledelayedexpansion

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Navigate to build directory
cd /d C:\zenith\daw\build

REM Run CMake with Visual Studio generator
cmake .. -G "Visual Studio 17 2022" -A x64 -DZENITH_ENABLE_SKIA=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
echo CMake Exit Code: \!ERRORLEVEL\!

REM List generated files
echo.
echo Generated files:
dir
