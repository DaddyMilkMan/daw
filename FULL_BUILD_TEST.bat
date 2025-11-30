@echo off
setlocal enabledelayedexpansion

echo.
echo ============================================
echo   ZENITH DAW - FULL BUILD WITH SKIA UI
echo ============================================
echo.

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

if errorlevel 1 (
    echo ERROR: Failed to initialize Visual Studio environment
    exit /b 1
)

cd /d C:\zenith\daw\build

echo.
echo === CMake Configuration ===
echo.
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe

if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed
    exit /b 1
)

echo.
echo === Building ZenithDAW ===
echo.
ninja ZenithDAW

if errorlevel 1 (
    echo.
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo ============================================
echo   BUILD SUCCESSFUL!
echo ============================================
echo.

REM Find the executable
for /r . %%F in (ZenithDAW.exe) do (
    echo Found: %%F
    set "ZENITH_EXE=%%F"
)

if defined ZENITH_EXE (
    echo.
    echo Executable ready at: %ZENITH_EXE%
    echo File size:
    dir "%ZENITH_EXE%"
) else (
    echo WARNING: Could not find ZenithDAW.exe
)

echo.
echo Build complete!
