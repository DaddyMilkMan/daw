@echo off
REM Reconfigure CMake and rebuild with Skia UI enabled

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

cd /d C:\zenith\daw\build

echo Cleaning build directory...
del /q /s * >nul 2>&1

echo.
echo Configuring with CMake...
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe

if errorlevel 1 (
    echo CMake configuration FAILED
    exit /b 1
)

echo.
echo Building...
ninja ZenithDAW 2>&1 | tail -n 50

if errorlevel 1 (
    echo Build FAILED
    exit /b 1
)

echo.
echo BUILD SUCCESSFUL!
