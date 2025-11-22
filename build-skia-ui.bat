@echo off
echo ============================================
echo BUILDING PURE SKIA UI
echo ============================================
cd /d C:\zenith\daw\build
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
ninja ZenithDAW
echo.
if %ERRORLEVEL% EQU 0 (
    echo ✓ Build successful!
    echo.
    echo Killing old instance...
    taskkill /F /IM "Zenith DAW.exe" 2>nul
    timeout /t 1 /nobreak >nul
    echo.
    echo Launching PURE SKIA UI...
    start "" "C:\zenith\daw\build\zenith-core\ZenithDAW_artefacts\Debug\Zenith DAW.exe"
    echo.
    echo ============================================
    echo 🎨 PURE SKIA RENDERING ACTIVE
    echo ============================================
    echo Look for in the logs:
    echo   - "ALL UI components rendered through Skia!"
    echo   - "NO MORE JUCE FALLBACK! Pure Skia UI!"
    echo.
    echo The green test box is GONE.
    echo Everything renders through beautiful Skia!
) else (
    echo ✗ Build failed!
)
