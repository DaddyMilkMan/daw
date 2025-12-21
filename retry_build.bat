@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
cd zenith-core
cmake --build build --verbose > build_log.txt 2>&1
type build_log.txt
