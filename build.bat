@echo off
echo ==========================================
echo Zenith DAW - Build Script
echo ==========================================

:: Check if CMake is available
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: CMake not found. Please install CMake and add it to your PATH.
    pause
    exit /b 1
)

:: Check if Ninja is available (for 'default' preset)
where ninja >nul 2>nul
if %errorlevel% neq 0 (
    echo Warning: Ninja not found. The 'default' CMake preset uses Ninja.
    echo         Consider installing Ninja for faster builds.
    echo.
)

:: Default build type is Release if not specified
set "BUILD_CONFIG=Release"
set "CMAKE_PRESET="

:: Parse arguments
:arg_loop
if "%1"=="" goto end_arg_loop
if /i "%1"=="--debug" (
    set "BUILD_CONFIG=Debug"
    set "CMAKE_PRESET=windows-msvc"
) else if /i "%1"=="--release" (
    set "BUILD_CONFIG=Release"
    set "CMAKE_PRESET=windows-msvc"
) else if /i "%1"=="--preset" (
    shift
    set "CMAKE_PRESET=%1"
)
shift
goto arg_loop
:end_arg_loop

:: If --preset was not specified, infer from config
if "%CMAKE_PRESET%"=="" (
    if /i "%BUILD_CONFIG%"=="Debug" (
        set "CMAKE_PRESET=windows-msvc"
    ) else (
        set "CMAKE_PRESET=default" :: Uses Ninja and Release by default
    )
)

echo Using CMake Preset: %CMAKE_PRESET%
echo Build Configuration: %BUILD_CONFIG%

:: Configure and Build
if "%CMAKE_PRESET%"=="default" (
    echo.
    echo Configuring with Ninja (Release)...
    cmake --preset default
    if %errorlevel% neq 0 (
        echo CMake configuration failed.
        pause
        exit /b %errorlevel%
    )
    
    echo.
    echo Building with Ninja...
    cmake --build --preset default
    if %errorlevel% neq 0 (
        echo Build failed.
        pause
        exit /b %errorlevel%
    )
) else (
    echo.
    echo Configuring with Visual Studio generator (%BUILD_CONFIG%)...
    cmake --preset %CMAKE_PRESET%
    if %errorlevel% neq 0 (
        echo CMake configuration failed.
        pause
        exit /b %errorlevel%
    )
    
    echo.
    echo Building for %BUILD_CONFIG%...
    cmake --build build --config %BUILD_CONFIG%
    if %errorlevel% neq 0 (
        echo Build failed.
        pause
        exit /b %errorlevel%
    )
)

echo.
echo Build process finished.
pause
