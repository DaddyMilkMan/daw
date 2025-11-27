@echo off
echo Configuring Zenith DAW build...
"C:\Program Files\CMake\bin\cmake.exe" -S "C:\zenith\daw\zenith-core" -B "C:\zenith\daw\build" -G Ninja -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% EQU 0 (
    echo Configuration successful!
) else (
    echo Configuration failed with error %ERRORLEVEL%
)
