@echo off
echo ===============================================
echo TEAM REBUILD - ALL FIXES APPLIED
echo ===============================================
echo.

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

cd /d C:\zenith\daw\build

echo [1/3] Reconfiguring CMake...
cmake .. -G Ninja -DZENITH_ENABLE_SKIA=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake 2>&1 | tail -n 20

echo.
echo [2/3] Building with Ninja...
echo (Showing compilation progress and errors)
echo.

ninja ZenithDAW 2>&1

echo.
echo [3/3] Checking result...
if exist modules\zenith-core\ZenithDAW.exe (
    echo.
    echo ===============================================
    echo BUILD SUCCESSFUL\!
    echo ===============================================
    dir modules\zenith-core\ZenithDAW.exe
) else (
    echo.
    echo Build failed - see errors above
    exit /b 1
)
