@echo off
REM ==============================================================================
REM Fetch and Build Google Crashpad (W8)
REM ==============================================================================
REM
REM This script automates the Crashpad build process:
REM   1. Clone depot_tools (Chromium build system)
REM   2. Fetch Crashpad source (~500 MB)
REM   3. Build crashpad_handler.exe with Visual Studio 2022 (x64 Release)
REM   4. Copy handler to scripts\windows\crashpad\bin\
REM
REM Prerequisites:
REM   - Visual Studio 2022 with C++ Desktop Development workload
REM   - Windows SDK 10 or later
REM   - Python 3.8+ (bundled with depot_tools, but system Python also works)
REM   - ~2 GB free disk space
REM   - ~1 hour build time (first build; subsequent builds much faster)
REM
REM Run from: x64 Native Tools Command Prompt for VS 2022
REM   OR: Regular cmd.exe (script will attempt to locate vcvarsall.bat)
REM
REM ==============================================================================

setlocal enabledelayedexpansion

echo.
echo ==============================================================================
echo   Crashpad Build Script for Zenith DAW
echo ==============================================================================
echo.
echo This will download and build Google Crashpad from source.
echo.
echo Requirements:
echo   - Visual Studio 2022 (C++ Desktop Development)
echo   - Windows SDK 10+
echo   - ~2 GB disk space
echo   - ~1 hour build time
echo.
echo Press Ctrl+C to cancel, or any key to continue...
pause >nul

REM ==============================================================================
REM Locate Visual Studio 2022
REM ==============================================================================

echo.
echo [1/6] Locating Visual Studio 2022...

set VS_PATH=
set VCVARSALL=

REM Check common VS 2022 install paths
for %%E in (Community Professional Enterprise BuildTools) do (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat" (
        set VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\%%E
        set VCVARSALL=!VS_PATH!\VC\Auxiliary\Build\vcvarsall.bat
        goto :vs_found
    )
)

echo [ERROR] Visual Studio 2022 not found.
echo.
echo Install from: https://visualstudio.microsoft.com/downloads/
echo Required workload: "Desktop development with C++"
echo.
pause
exit /b 1

:vs_found
echo [OK] Found Visual Studio 2022: !VS_PATH!

REM ==============================================================================
REM Set Up Environment
REM ==============================================================================

echo.
echo [2/6] Setting up build environment...

REM Check if already in VS Native Tools prompt
where cl.exe >nul 2>&1
if %errorLevel% equ 0 (
    echo [OK] Already in Visual Studio environment
) else (
    echo [INFO] Activating Visual Studio x64 environment...
    call "!VCVARSALL!" x64
    if %errorLevel% neq 0 (
        echo [ERROR] Failed to activate VS environment
        pause
        exit /b 1
    )
)

REM Verify compiler
cl.exe /? >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] MSVC compiler (cl.exe) not found
    pause
    exit /b 1
)

echo [OK] MSVC compiler ready

REM ==============================================================================
REM Create Working Directory
REM ==============================================================================

echo.
echo [3/6] Setting up working directory...

set SCRIPT_DIR=%~dp0
set WORK_DIR=%SCRIPT_DIR%_build_crashpad
set DEPOT_TOOLS_DIR=%WORK_DIR%\depot_tools
set CRASHPAD_DIR=%WORK_DIR%\crashpad

if not exist "%WORK_DIR%" (
    mkdir "%WORK_DIR%"
)

pushd "%WORK_DIR%"

echo [OK] Working directory: %WORK_DIR%

REM ==============================================================================
REM Clone depot_tools (Chromium Build System)
REM ==============================================================================

echo.
echo [4/6] Fetching depot_tools...

if not exist "%DEPOT_TOOLS_DIR%" (
    echo [INFO] Cloning depot_tools from Chromium repository...
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
    if %errorLevel% neq 0 (
        echo [ERROR] Failed to clone depot_tools
        popd
        pause
        exit /b 1
    )
) else (
    echo [INFO] depot_tools already exists, updating...
    pushd "%DEPOT_TOOLS_DIR%"
    git pull
    popd
)

REM Add depot_tools to PATH (temporary, for this session)
set PATH=%DEPOT_TOOLS_DIR%;%PATH%

REM Disable depot_tools auto-update (we just updated manually)
set DEPOT_TOOLS_UPDATE=0

echo [OK] depot_tools ready

REM ==============================================================================
REM Fetch Crashpad Source
REM ==============================================================================

echo.
echo [5/6] Fetching Crashpad source (~500 MB, may take 10-20 minutes)...

if not exist "%CRASHPAD_DIR%" (
    echo [INFO] Running: fetch crashpad
    echo [INFO] This downloads Crashpad and all dependencies...
    fetch crashpad
    if %errorLevel% neq 0 (
        echo [ERROR] Failed to fetch Crashpad
        popd
        pause
        exit /b 1
    )
) else (
    echo [INFO] Crashpad source already exists
    pushd "%CRASHPAD_DIR%\crashpad"
    echo [INFO] Syncing dependencies...
    gclient sync
    popd
)

pushd "%CRASHPAD_DIR%\crashpad"

echo [OK] Crashpad source ready: %CRASHPAD_DIR%\crashpad

REM ==============================================================================
REM Build Crashpad Handler (x64 Release)
REM ==============================================================================

echo.
echo [6/6] Building crashpad_handler.exe (x64 Release, ~30-60 minutes)...
echo [INFO] This may take a while on first build. Subsequent builds are much faster.
echo.

REM Generate build files with gn
if not exist "out\Release_x64" (
    echo [INFO] Generating build files with gn...
    gn gen out\Release_x64 --args="is_debug=false target_cpu=\"x64\" extra_cflags=\"/MD\""
    if %errorLevel% neq 0 (
        echo [ERROR] Failed to generate build files
        popd
        popd
        pause
        exit /b 1
    )
)

REM Build crashpad_handler.exe with ninja
echo [INFO] Building with ninja (this will take a while)...
ninja -C out\Release_x64 crashpad_handler
if %errorLevel% neq 0 (
    echo [ERROR] Build failed
    popd
    popd
    pause
    exit /b 1
)

REM Verify output
if not exist "out\Release_x64\crashpad_handler.exe" (
    echo [ERROR] crashpad_handler.exe not found after build
    popd
    popd
    pause
    exit /b 1
)

echo [OK] Build complete: out\Release_x64\crashpad_handler.exe

REM ==============================================================================
REM Copy Handler to bin Directory
REM ==============================================================================

echo.
echo [INFO] Copying crashpad_handler.exe to scripts\windows\crashpad\bin\...

set BIN_DIR=%SCRIPT_DIR%bin
if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"

copy /Y "out\Release_x64\crashpad_handler.exe" "%BIN_DIR%\crashpad_handler.exe"
if %errorLevel% neq 0 (
    echo [ERROR] Failed to copy handler
    popd
    popd
    pause
    exit /b 1
)

popd
popd

echo [OK] Handler copied to: %BIN_DIR%\crashpad_handler.exe

REM ==============================================================================
REM Success Summary
REM ==============================================================================

echo.
echo ==============================================================================
echo   Build Complete!
echo ==============================================================================
echo.
echo crashpad_handler.exe location:
echo   %BIN_DIR%\crashpad_handler.exe
echo.
echo File size:
for %%A in ("%BIN_DIR%\crashpad_handler.exe") do set FILE_SIZE=%%~zA
set /a SIZE_KB=!FILE_SIZE! / 1024
echo   !SIZE_KB! KB
echo.
echo ==============================================================================
echo   Next Steps
echo ==============================================================================
echo.
echo 1. Configure CMake with Crashpad enabled:
echo.
echo    cmake -B build -DZENITH_USE_CRASHPAD=ON ^
echo          -DCRAS HPAD_HANDLER_PATH="%BIN_DIR%\crashpad_handler.exe"
echo.
echo 2. Build Zenith DAW:
echo.
echo    cmake --build build --config Debug
echo.
echo 3. Test crash reporting:
echo.
echo    - Launch ZenithDAW.exe (Debug build)
echo    - Menu: Help -^> Diagnostics -^> Trigger Test Crash
echo    - Check for .dmp in: %%APPDATA%%\ZenithDAW\crashpad_db\completed\
echo.
echo ==============================================================================
echo.

REM Copy full path to clipboard (requires clip.exe, available on Windows 10+)
where clip.exe >nul 2>&1
if %errorLevel% equ 0 (
    echo %BIN_DIR%\crashpad_handler.exe | clip
    echo [INFO] Full handler path copied to clipboard!
    echo.
)

pause
endlocal
