@echo off
cd /d C:\zenith\daw\build
echo Configuring with Visual Studio 2022 generator...
cmake .. -G "Visual Studio 17 2022" -A x64 -DZENITH_ENABLE_SKIA=OFF -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

if errorlevel 1 (
    echo CMake configuration failed
    exit /b 1
)

echo.
echo Building with MSBuild...
cmake --build . --target ZenithDAW --config Release 2>&1 | tail -n 80
