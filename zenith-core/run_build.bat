@echo off
cd /d C:\zenith\daw\zenith-core
echo Starting CMake configure...
"C:\Program Files\CMake\bin\cmake.exe" -B build -G "Visual Studio 17 2022" -A x64 > cmake_log.txt 2>&1
echo CMake configure exit code: %ERRORLEVEL% >> cmake_log.txt
echo.
echo Starting build...
"C:\Program Files\CMake\bin\cmake.exe" --build build --config Release >> cmake_log.txt 2>&1
echo Build exit code: %ERRORLEVEL% >> cmake_log.txt
echo Done!
