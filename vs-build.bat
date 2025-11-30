@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

cd /d C:\zenith\daw\build
echo Configuring with Visual Studio generator...
cmake .. -G "Visual Studio 18 2026" -A x64 -DZENITH_ENABLE_SKIA=OFF -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

if errorlevel 1 (
    echo CMake configuration failed
    exit /b 1
)

echo.
echo Building with MSBuild...
cmake --build . --target ZenithDAW --config Release
