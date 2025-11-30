@echo off
REM Try VS2022 first, fallback to VS2026
WHERE cl.exe >nul 2>&1
IF %ERRORLEVEL% EQU 0 (
    echo Using existing compiler in PATH
    cl.exe 2>&1 | findstr Version
) ELSE (
    IF EXIST "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        echo Found VS2022 Community
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) ELSE (
        echo Using VS2026
        call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    )
)

cd /d C:\zenith\daw
rd /s /q build 2>nul
mkdir build
cd build

echo Configuring with auto-detected compiler...
cmake .. -G Ninja -DZENITH_ENABLE_SKIA=OFF -DCMAKE_BUILD_TYPE=Release
