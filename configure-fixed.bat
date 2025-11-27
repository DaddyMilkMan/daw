@echo off
cd zenith-core
cmake -B ..\build -G Ninja -DCMAKE_BUILD_TYPE=Release
cd ..
