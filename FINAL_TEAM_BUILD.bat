@echo off
setlocal enabledelayedexpansion

echo ===============================================
echo TEAM FINAL BUILD - WITH FILE FIXES
echo ===============================================

cd /d C:\zenith\daw

echo [1/5] Applying file fixes...
timeout /t 2 /nobreak >nul
copy /Y MainWindow_FIXED.h modules\zenith-core\include\MainWindow.h >nul 2>&1
if errorlevel 1 (
    echo WARNING: Could not copy MainWindow.h - file may be locked
) else (
    echo   ? MainWindow.h fixed
)

copy /Y AutomationLaneComponent_FIXED.cpp modules\zenith-core\src\AutomationLaneComponent.cpp >nul 2>&1
if errorlevel 1 (
    echo WARNING: Could not copy AutomationLaneComponent.cpp - file may be locked
) else (
    echo   ? AutomationLaneComponent.cpp fixed
)

echo.
echo [2/5] Setting up Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

echo [3/5] Cleaning and reconfiguring...
cd build
cmake .. -G Ninja -DZENITH_ENABLE_SKIA=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake >nul 2>&1

echo [4/5] Building (showing progress)...
ninja ZenithDAW 2>&1 | findstr /I /C:"[" /C:"error" /C:"FAILED"

if exist modules\zenith-core\ZenithDAW.exe (
    echo.
    echo ===============================================
    echo ??? BUILD SUCCESSFUL ???
    echo ===============================================
    dir modules\zenith-core\ZenithDAW.exe
) else (
    echo.
    echo Build completed with errors - check output above
)
