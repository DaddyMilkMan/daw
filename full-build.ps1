# Load VS environment
& 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64

# Reconfigure
Set-Location C:\zenith\daw\build
& 'C:\Program Files\CMake\bin\cmake.exe' -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..

# Build
ninja -j8
