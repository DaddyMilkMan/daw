@echo off
REM Quick rebuild with CMake reconfiguration

echo Rebuilding Zenith DAW...

cd /d C:\zenith\daw\build

REM Reconfigure CMake (this will pick up the new CMakeLists.txt and SkiaManualIntegration.cmake)
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe

echo.
echo Building...
ninja ZenithDAW

if errorlevel 1 (
    echo Build FAILED
    exit /b 1
)

echo.
echo BUILD SUCCESSFUL!
echo.

REM Find executable
dir /s /b ZenithDAW.exe 2>nul | head -1

echo.
echo To run: cd build && zenith-core\ZenithDAW_artefacts\Debug\ZenithDAW.exe
