@echo off
echo Copying latest build artifacts...
copy /Y "c:\zenith\daw\build\modules\zenith-core\ZenithDAW_artefacts\Debug\Zenith DAW.exe" "c:\zenith\daw\ZenithDAW.exe"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to copy executable!
    pause
    exit /b 1
)

echo Launching Zenith DAW (Diagnostic Mode)...
ZenithDAW.exe > stdout.txt 2> stderr.txt
echo %ERRORLEVEL% > exitcode.txt
echo Exit Code: %ERRORLEVEL%

echo.
echo STDOUT:
type stdout.txt
echo.
echo STDERR:
type stderr.txt

pause
