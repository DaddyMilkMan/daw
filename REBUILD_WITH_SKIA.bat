@echo off
REM ============================================================================
REM Rebuild Zenith DAW with Skia UI Enabled
REM ============================================================================

echo.
echo ============================================
echo   REBUILD WITH SKIA UI
echo ============================================
echo.

REM Set up Visual Studio 2026 environment
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

if errorlevel 1 (
    echo ERROR: Failed to set up Visual Studio environment
    exit /b 1
)

REM Clean previous build
echo Cleaning previous build...
cd /d C:\zenith\daw
rd /s /q build 2>nul
mkdir build
cd build

echo.
echo ============================================
echo   CMAKE CONFIGURATION
echo ============================================
echo.
echo Building WITH Skia UI enabled...
echo.

REM Configure with Skia ENABLED
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=ON

if errorlevel 1 (
    echo.
    echo ============================================
    echo   CMAKE CONFIGURATION FAILED
    echo ============================================
    echo.
    echo This likely means Skia is not installed.
    echo.
    echo Falling back to JUCE UI mode...
    echo.

    REM Try again with Skia DISABLED
    cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=OFF

    if errorlevel 1 (
        echo ERROR: CMake configuration failed even without Skia
        exit /b 1
    )

    echo.
    echo ============================================
    echo   BUILDING WITH JUCE FALLBACK UI
    echo ============================================
    echo.
) else (
    echo.
    echo ============================================
    echo   BUILDING WITH SKIA UI
    echo ============================================
    echo.
)

REM Build
ninja ZenithDAW

if errorlevel 1 (
    echo.
    echo ============================================
    echo   BUILD FAILED
    echo ============================================
    exit /b 1
)

echo.
echo ============================================
echo   BUILD SUCCESSFUL!
echo ============================================
echo.

REM Find the executable
if exist "zenith-core\ZenithDAW_artefacts\Debug\ZenithDAW.exe" (
    echo Executable: build\zenith-core\ZenithDAW_artefacts\Debug\ZenithDAW.exe
    dir "zenith-core\ZenithDAW_artefacts\Debug\ZenithDAW.exe"
) else if exist "zenith-core\ZenithDAW_artefacts\Release\ZenithDAW.exe" (
    echo Executable: build\zenith-core\ZenithDAW_artefacts\Release\ZenithDAW.exe
    dir "zenith-core\ZenithDAW_artefacts\Release\ZenithDAW.exe"
) else (
    echo Searching for executable...
    dir /s /b "ZenithDAW.exe" 2>nul
)

echo.
echo Done!
