@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d C:\zenith\daw
rd /s /q build 2>nul
mkdir build
cd build

echo ============================================
echo   REBUILDING WITH SKIA ENABLED
echo ============================================
echo.
echo Configuring CMake with Skia support...
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed\!
    exit /b 1
)

echo.
echo Building ZenithDAW with Skia...
ninja ZenithDAW

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo   BUILD SUCCESS WITH SKIA\!
    echo ============================================
    dir modules\zenith-core\ZenithDAW_artefacts\Debug\*.exe
) else (
    echo.
    echo Build failed. Checking errors...
)
