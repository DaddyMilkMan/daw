@echo off
echo Setting up Visual Studio 2026 environment...
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

if errorlevel 1 (
    echo Failed to set up Visual Studio environment
    exit /b 1
)

echo Environment set up successfully
where cl.exe

cd /d C:\zenith\daw
echo Cleaning build directory...
rmdir /s /q build 2>nul
mkdir build
cd build

echo.
echo Configuring with CMake (Skia enabled)...
cmake .. -G Ninja -DZENITH_ENABLE_SKIA=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

if errorlevel 1 (
    echo CMake configuration failed
    exit /b 1
)

echo.
echo Building ZenithDAW...
ninja ZenithDAW 2>&1

echo.
echo Build complete\! Exit code: %ERRORLEVEL%
if exist zenith-core\ZenithDAW.exe (
    echo.
    echo ============================================
    echo SUCCESS: ZenithDAW.exe built successfully\!
    echo ============================================
    dir zenith-core\ZenithDAW.exe
)
