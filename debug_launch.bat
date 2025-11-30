@echo off
echo Launching Zenith DAW...
ZenithDAW.exe > stdout.txt 2> stderr.txt
echo Exit Code: %ERRORLEVEL%
type stdout.txt
type stderr.txt
pause
