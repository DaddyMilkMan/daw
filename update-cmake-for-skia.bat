@echo off
echo ============================================
echo Updating CMakeLists.txt for Manual Skia
echo ============================================
echo.

cd /d "%~dp0"

REM Check if Python is available
python --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Running Python script...
    python update-cmake-for-skia.py
    exit /b %ERRORLEVEL%
)

echo Python not found, trying manual method...
echo.

cd /d "%~dp0zenith-core"

echo Backing up original CMakeLists.txt...
copy /Y CMakeLists.txt CMakeLists.txt.backup

echo.
echo Manually updating CMakeLists.txt...
echo This will replace the Skia section with a simpler include statement.
echo.

REM Create a simple replacement using a temporary file
powershell -ExecutionPolicy Bypass -Command ^
"$content = Get-Content 'CMakeLists.txt' -Raw; ^
$pattern = '(?s)# =+\r?\n# Skia Rendering \(conditional\)\r?\n# =+.*?endif\(\)'; ^
$replacement = '# ============================================================================`r`n# Skia Rendering (conditional) - Manual Integration`r`n# ============================================================================`r`ninclude($${CMAKE_CURRENT_SOURCE_DIR}/../cmake/SkiaManualIntegration.cmake)'; ^
$newContent = $content -replace $pattern, $replacement; ^
if ($newContent -eq $content) { ^
    Write-Host 'ERROR: Could not find Skia section!' -ForegroundColor Red; ^
    exit 1; ^
} ^
Set-Content 'CMakeLists.txt' -Value $newContent -NoNewline -Encoding UTF8; ^
Write-Host 'CMakeLists.txt updated successfully!' -ForegroundColor Green"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Failed to update CMakeLists.txt
    echo Restoring backup...
    copy /Y CMakeLists.txt.backup CMakeLists.txt
    echo.
    echo Please update manually:
    echo 1. Open: C:\zenith\daw\zenith-core\CMakeLists.txt
    echo 2. Find the "Skia Rendering (conditional)" section (around line 195)
    echo 3. Replace the entire if(ZENITH_ENABLE_SKIA)...endif() block with:
    echo.
    echo # ============================================================================
    echo # Skia Rendering (conditional) - Manual Integration
    echo # ============================================================================
    echo include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/SkiaManualIntegration.cmake)
    echo.
    pause
    exit /b 1
)

echo.
echo ============================================
echo CMakeLists.txt Updated Successfully!
echo ============================================
echo.
echo Original saved as: CMakeLists.txt.backup
echo.
echo Next: Run build-with-manual-skia.bat
echo.
pause
