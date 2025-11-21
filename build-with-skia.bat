@echo off
REM Build Zenith DAW with Skia GPU rendering enabled

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Clean previous build
echo Cleaning previous build...
rd /s /q C:\zenith\daw\build

REM Create build directory
mkdir C:\zenith\daw\build
cd /d C:\zenith\daw\build

REM Configure with Skia enabled
echo Configuring CMake with Skia enabled...
cmake .. -G Ninja -DZENITH_ENABLE_SKIA=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

REM Build
echo Building Zenith DAW with Skia...
ninja ZenithDAW

if %ERRORLEVEL% == 0 (
    echo.
    echo ==========================================
    echo Build completed successfully!
    echo ==========================================
    echo.
    echo Executable location:
    dir /s /b ZenithDAW.exe
    echo.
    echo Your Zenith DAW now has:
    echo - GPU-accelerated Skia rendering
    echo - Flashy text with glows and shadows
    echo - Professional waveform visualization
    echo - Spring physics animations
    echo - Dark/Light themes
    echo - 3D depth effects
    echo.
) else (
    echo.
    echo Build FAILED with error code %ERRORLEVEL%
    echo.
)

pause
