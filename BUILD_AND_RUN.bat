@echo off
echo ============================================
echo BUILDING ZENITH DAW WITH VISUAL STUDIO 2026
echo ============================================
echo.

REM Initialize VS 2026 build environment
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64

echo.
echo Cleaning old build...
cd /d "%~dp0zenith-core"
if exist "out" rmdir /s /q out
if exist "build" rmdir /s /q build

echo.
echo Configuring with CMake...
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Release -DZENITH_ENABLE_SKIA=OFF

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Building Release configuration...
echo This will take 5-15 minutes...
echo.

cmake --build build --config Release --parallel 4 --verbose

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo BUILD FAILED!
    echo Check errors above.
    pause
    exit /b 1
)

echo.
echo ============================================
echo BUILD SUCCESSFUL!
echo ============================================
echo.

REM Find the executable
echo Looking for ZenithDAW.exe...
for /r build %%i in (*.exe) do (
    echo Found: %%i
    if /i "%%~ni"=="ZenithDAW" (
        set EXE_PATH=%%i
    )
)

if defined EXE_PATH (
    echo.
    echo Executable location:
    echo %EXE_PATH%
    echo.
    echo Launching Zenith DAW...
    start "" "%EXE_PATH%"
) else (
    echo.
    echo Could not find ZenithDAW.exe
    echo Searching all .exe files:
    dir /s /b build\*.exe
)

echo.
pause
