@echo off
echo ===============================================
echo TEAM BUILD ATTEMPT - FULL SKIA INTEGRATION
echo ===============================================
echo.

REM Set up Visual Studio 2026 environment
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

cd /d C:\zenith\daw

echo [1/4] Cleaning old build...
rmdir /s /q build 2>nul
mkdir build
cd build

echo.
echo [2/4] Configuring CMake with Skia enabled...
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake 2>&1 | tail -n 30

if errorlevel 1 (
    echo.
    echo [ERROR] CMake configuration failed\!
    exit /b 1
)

echo.
echo [3/4] Building with Ninja...
echo (This will take a while... showing errors only)
echo.

ninja ZenithDAW 2>&1 | findstr /I /C:"error" /C:"FAILED" /C:"fatal" /C:"Building CXX" /C:"Linking"

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed\!
) else (
    echo.
    echo ===============================================
    echo BUILD SUCCESSFUL\!
    echo ===============================================
    if exist zenith-core\ZenithDAW.exe (
        dir zenith-core\ZenithDAW.exe
    )
)
