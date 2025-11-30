@echo off
echo Setting up Visual Studio 2026 environment...
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

if errorlevel 1 (
    echo Failed to set up Visual Studio environment
    exit /b 1
)

cd /d C:\zenith\daw\build
echo.
echo Configuring CMake...
cmake .. -G Ninja -DZENITH_ENABLE_SKIA=OFF -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

if errorlevel 1 (
    echo CMake configuration failed
    exit /b 1
)

echo.
echo Building ZenithDAW...
ninja ZenithDAW 2>&1 | tail -n 80
