@echo off
echo ============================================
echo Building Zenith DAW WITHOUT Skia
echo ============================================
echo.
echo This build will:
echo 1. Disable Skia rendering (use JUCE fallback)
echo 2. Build with JUCE 8.0.9 only
echo.

cd /d "%~dp0"

REM Clean previous build
if exist "build" (
    echo Cleaning previous build...
    rmdir /s /q build
)

REM Create build directory
mkdir build
cd build

echo.
echo Configuring with CMake...
echo.

REM Configure with Skia DISABLED
cmake .. -G "Visual Studio 17 2022" -A x64 -DZENITH_ENABLE_SKIA=OFF

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ============================================
    echo CMAKE CONFIGURATION FAILED!
    echo ============================================
    pause
    exit /b 1
)

echo.
echo ============================================
echo Configuration successful!
echo ============================================
echo.
echo Building in Release mode...
echo.

REM Build in Release mode
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ============================================
    echo BUILD FAILED!
    echo ============================================
    pause
    exit /b 1
)

echo.
echo ============================================
echo BUILD SUCCESSFUL!
echo ============================================
echo.
echo Executable location:
echo %CD%\ZenithDAW_artefacts\Release\Zenith DAW.exe
echo.
pause
