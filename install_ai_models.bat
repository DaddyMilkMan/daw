@echo off
setlocal
title Zenith DAW - AI Model Installer

echo ========================================================
echo   Zenith DAW - AI Model Installer and Converter
echo ========================================================
echo.
echo This script will:
echo 1. Check for Python and Git
echo 2. Clone the required conversion tools (lightweight)
echo 3. Install dependencies
echo 4. Convert the Demucs v4 Model to ONNX
echo 5. Install it into Zenith DAW
echo.
pause

:: 1. Check Prerequisites
echo.
echo [1/5] Checking prerequisites...

:: Check Git
where git >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] Git is not found. Please install Git: https://git-scm.com/download/win
    pause
    exit /b 1
)

:: Check Python (Robust)
set "PYTHON_CMD=python"

:: 1. Try standard command
python --version >nul 2>nul
if %errorlevel% equ 0 goto :FoundPython

:: 2. Try 'py' launcher
py --version >nul 2>nul
if %errorlevel% equ 0 (
    set "PYTHON_CMD=py"
    goto :FoundPython
)

:: 3. Try common paths
if exist "C:\Python311\python.exe" (
    set "PYTHON_CMD=C:\Python311\python.exe"
    goto :FoundPython
)
if exist "%LOCALAPPDATA%\Programs\Python\Python311\python.exe" (
    set "PYTHON_CMD=%LOCALAPPDATA%\Programs\Python\Python311\python.exe"
    goto :FoundPython
)

:: If we get here, we really can't find it
echo [ERROR] Python was not found in PATH or common locations.
echo.
echo You said Python 3.11 is installed. It might not be in your PATH.
echo.
echo Please try:
echo 1. Open Start Menu -> Search "Manage App Execution Aliases"
echo 2. Turn OFF the toggles for "App Installer (python.exe)"
echo 3. Re-run this script.
echo.
pause
exit /b 1

:FoundPython
echo Found Python: %PYTHON_CMD%

:: 2. Setup Workspace
set "WORK_DIR=%~dp0temp_ai_setup"
set "DEST_DIR=%~dp0zenith-core\Resources\models"

if not exist "%DEST_DIR%" mkdir "%DEST_DIR%"
if exist "%WORK_DIR%" rd /s /q "%WORK_DIR%"
mkdir "%WORK_DIR%"
cd "%WORK_DIR%"

:: 3. Clone Repository (Lightweight - No Submodules)
echo.
echo [2/5] Cloning conversion tools...
:: We only need the python scripts, not the heavy C++ submodules
git clone --depth 1 https://github.com/sevagh/demucs.onnx.git
cd demucs.onnx

:: 4. Setup Python Venv
echo.
echo [3/5] Setting up Python environment...
%PYTHON_CMD% -m venv venv
call venv\Scripts\activate

echo Installing dependencies...
:: Install demucs directly
pip install demucs
:: Install torch (CPU version is fine for export)
pip install torch torchaudio --index-url https://download.pytorch.org/whl/cpu
:: Install onnx
pip install onnx

:: 5. Run Conversion
echo.
echo [4/5] Downloading and Converting Model (htdemucs)...
echo This step downloads the weights from Facebook and converts them.
:: We use our custom script which handles the export robustly
if not exist "onnx-models" mkdir "onnx-models"
:: IMPORTANT: Use the VENV python to ensure we have access to installed dependencies
set PYTHONUTF8=1
venv\Scripts\python.exe scripts\convert-pth-to-onnx.py onnx-models

:: 6. Install
echo.
echo [5/5] Installing model to Zenith...
if exist "onnx-models\htdemucs.onnx" (
    copy "onnx-models\*.*" "%DEST_DIR%\"
    echo.
    echo [SUCCESS] Model installed to: %DEST_DIR%\htdemucs.onnx
) else (
    echo.
    echo [ERROR] Conversion failed. Output file not found.
    echo Check the output above for Python errors.
    pause
    exit /b 1
)

:: 7. Cleanup
echo.
echo Cleaning up temporary files...
cd ..\..
rd /s /q "%WORK_DIR%"

echo.
echo ========================================================
echo   INSTALLATION COMPLETE
echo ========================================================
echo.
echo You can now restart Zenith DAW.
echo The 'separate_stems' command will now use the High-Quality AI backend.
echo.
pause
