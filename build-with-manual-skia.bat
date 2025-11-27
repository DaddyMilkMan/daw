@echo off
echo ============================================
echo Building Zenith DAW with Manual Skia
echo ============================================
echo.

cd /d "%~dp0zenith-core"

REM Check if Skia exists
if not exist "C:\zenith\skia" (
    echo ERROR: Skia not found at C:\zenith\skia
    echo.
    echo Please run: download-skia.bat first
    echo.
    pause
    exit /b 1
)

REM Check if CMakeLists has been updated
findstr /C:"SkiaManualIntegration.cmake" CMakeLists.txt >nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMakeLists.txt has not been updated for manual Skia
    echo.
    echo Please run: update-cmake-for-skia.bat first
    echo.
    pause
    exit /b 1
)

echo All prerequisites met!
echo.

REM Clean build
if exist "build" (
    echo Cleaning previous build...
    rmdir /s /q build
)

mkdir build
cd build

echo.
echo ============================================
echo Configuring CMake with Manual Skia...
echo ============================================
echo.

cmake .. -G "Visual Studio 17 2022" -A x64 -DZENITH_ENABLE_SKIA=ON -DSKIA_DIR="C:/zenith/skia"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ============================================
    echo CMAKE CONFIGURATION FAILED!
    echo ============================================
    echo.
    pause
    exit /b 1
)

echo.
echo ============================================
echo Building Release with Skia...
echo ============================================
echo.

cmake --build . --config Release --parallel

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ============================================
    echo BUILD FAILED!
    echo ============================================
    echo.
    echo Check the error messages above.
    echo.
    pause
    exit /b 1
)

echo.
echo ============================================
echo BUILD SUCCESSFUL WITH SKIA!
echo ============================================
echo.
echo Executable: %CD%\zenith-core\ZenithDAW_artefacts\Release\Zenith DAW.exe
echo.
echo Skia GPU rendering is now enabled!
echo.
pause
