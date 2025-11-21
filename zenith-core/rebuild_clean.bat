@echo off
echo ========================================
echo Zenith DAW - Clean Rebuild
echo ========================================

REM Find Visual Studio 2022
set VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat

if exist "%VS_PATH%" (
    echo Found Visual Studio 2022
    call "%VS_PATH%" -arch=x64 -host_arch=x64
) else (
    echo WARNING: Visual Studio 2022 not found at expected location
    echo Attempting build anyway...
)

echo.
echo [1/3] Cleaning build directory...
if exist build (
    echo Removing old build files...
    rmdir /s /q build
)

echo.
echo [2/3] Configuring with CMake...
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DZENITH_ENABLE_SKIA=ON

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Configuration failed!
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo [3/3] Building project...
cmake --build build --config Debug --parallel 8

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed!
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.
pause
