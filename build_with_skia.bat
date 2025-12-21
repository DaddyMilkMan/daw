@echo off
echo ============================================
echo BUILDING ZENITH DAW (with Skia via vcpkg)
echo ============================================
echo.

call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64

cd zenith-core
if exist build rmdir /s /q build

echo Configuring with Skia enabled...
cmake -B build -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DZENITH_ENABLE_SKIA=ON ^
    -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

if %ERRORLEVEL% NEQ 0 (
    echo CMake Configuration Failed!
    pause
    exit /b 1
)

echo.
echo Building...
cmake --build build --verbose --parallel 4

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo BUILD SUCCESSFUL WITH SKIA!
    echo ============================================
    echo.
    echo Finding executable...
    for /r build %%i in (*.exe) do (
        echo Found: %%i
        start "" "%%i"
    )
) else (
    echo.
    echo BUILD FAILED!
)
pause
