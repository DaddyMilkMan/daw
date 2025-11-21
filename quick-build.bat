@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d C:\zenith\daw\build
ninja -j8
if %ERRORLEVEL% == 0 (
    echo Build completed successfully!
    echo Executable location:
    dir /s /b ZenithDAW.exe
) else (
    echo Build failed with error code %ERRORLEVEL%
)
