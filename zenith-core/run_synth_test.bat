@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
cmake --build build_synth_test --target SynthHeadlessTest --config Debug > build_log.txt 2>&1
type build_log.txt
