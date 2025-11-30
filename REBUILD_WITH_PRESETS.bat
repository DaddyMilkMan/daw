@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d C:\zenith\daw\build
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe -DZENITH_ENABLE_SKIA=OFF -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake >nul 2>&1
echo ============================================
echo REBUILD WITH PRESET MANAGER ENABLED
echo ============================================
ninja ZenithDAW 2>&1 | findstr /C:"error" /C:"FAILED" /C:"Linking" /C:"built successfully" | tail -n 20
