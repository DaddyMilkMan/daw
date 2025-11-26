@echo off
echo ============================================
echo Downloading Pre-built Skia for Windows
echo ============================================
echo.

set SKIA_DIR=C:\zenith\skia
set SKIA_VERSION=m138-80d088a-1

echo Creating Skia directory: %SKIA_DIR%
if not exist "%SKIA_DIR%" mkdir "%SKIA_DIR%"

cd /d "%SKIA_DIR%"

echo.
echo Downloading Skia %SKIA_VERSION% from JetBrains...
echo This may take 2-5 minutes (~100MB download)...
echo.

REM Direct download URL for Windows Release x64
set DOWNLOAD_URL=https://github.com/JetBrains/skia-pack/releases/download/%SKIA_VERSION%/Skia-%SKIA_VERSION%-windows-Release-x64.zip

echo Downloading from:
echo %DOWNLOAD_URL%
echo.

REM Try PowerShell download first
powershell -Command "& { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; $ProgressPreference = 'SilentlyContinue'; Write-Host 'Downloading...'; Invoke-WebRequest -Uri '%DOWNLOAD_URL%' -OutFile 'skia.zip' -UseBasicParsing; Write-Host 'Download complete!' }"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo PowerShell download failed, trying curl...
    echo.
    
    REM Try curl with follow redirects
    curl -L --progress-bar -o skia.zip "%DOWNLOAD_URL%"
    
    if %ERRORLEVEL% NEQ 0 (
        echo.
        echo ERROR: All download methods failed!
        echo.
        echo Please download manually:
        echo 1. Open in browser: https://github.com/JetBrains/skia-pack/releases
        echo 2. Find: Skia-%SKIA_VERSION%-windows-Release-x64.zip
        echo 3. Download and extract to: %SKIA_DIR%
        echo.
        pause
        exit /b 1
    )
)

REM Check if file exists
if not exist "skia.zip" (
    echo.
    echo ERROR: Download file not found!
    echo.
    pause
    exit /b 1
)

REM Check file size
echo.
echo Verifying download...
for %%A in (skia.zip) do set SIZE=%%~zA
echo File size: %SIZE% bytes

if %SIZE% LSS 10000000 (
    echo.
    echo ERROR: Downloaded file is too small ^(only %SIZE% bytes^)
    echo This is probably an error page, not the actual Skia package.
    echo.
    echo Please download manually from:
    echo https://github.com/JetBrains/skia-pack/releases
    echo.
    del skia.zip
    pause
    exit /b 1
)

echo Download verified! File looks good.
echo.
echo Extracting Skia...

REM Extract using PowerShell
powershell -Command "& { Write-Host 'Extracting...'; Expand-Archive -Path 'skia.zip' -DestinationPath '.' -Force; Write-Host 'Extraction complete!' }"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo PowerShell extraction failed, trying tar...
    echo.
    
    REM Try using tar (available on Windows 10+)
    tar -xf skia.zip
    
    if %ERRORLEVEL% NEQ 0 (
        echo.
        echo ERROR: Both extraction methods failed!
        echo.
        echo Please extract skia.zip manually:
        echo 1. Right-click skia.zip
        echo 2. Select "Extract All..."
        echo 3. Extract to: %SKIA_DIR%
        echo.
        pause
        exit /b 1
    )
)

echo.
echo Cleaning up...
del skia.zip

echo.
echo Verifying installation...
echo.

REM Check for headers
if exist "include\core\SkCanvas.h" (
    echo [OK] Headers found: include\core\SkCanvas.h
) else (
    echo [ERROR] Headers not found!
    echo Expected: %SKIA_DIR%\include\core\SkCanvas.h
    echo.
    echo Directory contents:
    dir /b /s | findstr /i "SkCanvas"
    echo.
    pause
    exit /b 1
)

REM Check for library
if exist "out\Release-x64\skia.lib" (
    echo [OK] Library found: out\Release-x64\skia.lib
) else (
    echo [WARNING] Library not found at expected location
    echo Looking for skia.lib...
    dir /b /s skia.lib 2>nul
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] No skia.lib found anywhere!
        pause
        exit /b 1
    )
)

echo.
echo ============================================
echo Skia Downloaded Successfully!
echo ============================================
echo.
echo Location: %SKIA_DIR%
echo Version: %SKIA_VERSION%
echo.
echo Headers: %SKIA_DIR%\include\core\SkCanvas.h
echo Library: %SKIA_DIR%\out\Release-x64\skia.lib
echo.
echo Next step: Run update-cmake-for-skia.bat
echo.
pause
