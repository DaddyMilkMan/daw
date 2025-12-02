@echo off
setlocal
cd /d "%~dp0.."

echo [INFO] Zenith DAW Build Script
echo [INFO] Root: %CD%

if not exist build (
    echo [INFO] Creating build directory...
    mkdir build
)

cd build

echo [INFO] Configuring CMake...
cmake .. -DZENITH_USE_SKIA=1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake Configuration Failed!
    exit /b %ERRORLEVEL%
)

echo [INFO] Patching vcxproj to disable Spectre Mitigation...
powershell -Command "(Get-Content ZenithDAW.vcxproj) -replace '<SpectreMitigation>Spectre</SpectreMitigation>', '' | Set-Content ZenithDAW.vcxproj"

echo [INFO] Building Project...
cmake --build . --config Release -j 8
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build Failed!
    exit /b %ERRORLEVEL%
)

echo [SUCCESS] Build Complete!
endlocal