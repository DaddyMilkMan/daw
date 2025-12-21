@echo off
call C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat x64 >nul 2>&1

cd /d C:\zenith\daw
echo ============================================
echo CLEANING BUILD DIRECTORY
echo ============================================
rmdir /s /q build 2>nul
mkdir build
cd build

echo.
echo ============================================
echo RUNNING CMAKE CONFIGURATION
echo ============================================
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

if errorlevel 1 (
    echo CMAKE CONFIGURATION FAILED
    exit /b 1
)

echo.
echo ============================================
echo BUILDING WITH NINJA (showing last 100 lines)
echo ============================================
ninja ZenithDAW 2>&1 | tail -100

if errorlevel 1 (
    echo.
    echo BUILD FAILED\!
    exit /b 1
) else (
    echo.
    echo ============================================
    echo BUILD SUCCESSFUL\!
    echo ============================================
    if exist zenith-core\ZenithDAW.exe (
        echo Found executable: zenith-core\ZenithDAW.exe
        dir zenith-core\ZenithDAW.exe
    )
)
