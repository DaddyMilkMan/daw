@echo off
echo ============================================
echo SKIA RENDERING DEBUG UTILITY
echo ============================================
echo.
echo This batch file helps diagnose Skia rendering issues
echo.
echo [1] Rebuilding application...
cd /d C:\zenith\daw\build
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
ninja ZenithDAW 2>&1 | findstr /C:"Building" /C:"Linking" /C:"error" /C:"FAILED"
if %ERRORLEVEL% EQU 0 (
    echo     Build complete\!
) else (
    echo     Build had warnings/errors
)
echo.
echo [2] Killing any running instances...
taskkill /F /IM "Zenith DAW.exe" 2>nul
timeout /t 1 /nobreak >nul
echo     Done
echo.
echo [3] Launching application...
start "" "C:\zenith\daw\build\zenith-core\ZenithDAW_artefacts\Debug\Zenith DAW.exe"
timeout /t 3 /nobreak >nul
echo     Launched
echo.
echo [4] Instructions:
echo     - Look at the log display (top-right corner)
echo     - Find the [DEBUG] messages
echo     - Check if SkSurface pointer is valid (not 0)
echo     - Check if image dimensions match window size
echo     - Check if pixel copy succeeded
echo.
echo [5] Common issues:
echo     - If you see JUCE fallback: Child components covering Skia
echo     - If dimensions wrong: Resize triggered before paint
echo     - If no green rectangle: Being painted in covered area
echo.
echo ============================================
echo Press any key to close this window...
pause >nul
