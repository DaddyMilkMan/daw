@echo off
REM ==============================================================================
REM ETW Capture Script for Zenith DAW (W7)
REM ==============================================================================
REM
REM Usage: run_etw_capture.cmd <preset> [duration_seconds]
REM
REM Presets:
REM   debug-light  - General debugging (CPU, GPU, lightweight)
REM   glitch-hunt  - Audio dropout/glitch analysis
REM   gpu-ui       - GPU/DWM/Present performance
REM   audio-stack  - Full audio subsystem analysis
REM
REM Duration: Optional (seconds). If omitted, runs until Ctrl+C
REM
REM Output: logs\etw\YYYYMMDD_HHMMSS\session.etl
REM
REM Prerequisites:
REM   - Run as Administrator
REM   - Windows Performance Toolkit installed (wpr.exe in PATH)
REM
REM ==============================================================================

setlocal enabledelayedexpansion

REM ==============================================================================
REM Check Administrator Privileges
REM ==============================================================================

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo.
    echo ==============================================================================
    echo   ERROR: Administrator Privileges Required
    echo ==============================================================================
    echo.
    echo This script must be run as Administrator to capture ETW traces.
    echo.
    echo Right-click on Command Prompt and select "Run as Administrator"
    echo.
    pause
    exit /b 1
)

REM ==============================================================================
REM Parse Arguments
REM ==============================================================================

if "%~1"=="" (
    echo.
    echo ==============================================================================
    echo   Zenith DAW - ETW Capture Script
    echo ==============================================================================
    echo.
    echo Usage: run_etw_capture.cmd ^<preset^> [duration_seconds]
    echo.
    echo Presets:
    echo   debug-light  - General debugging (CPU, GPU, ~500 MB / 30s)
    echo   glitch-hunt  - Audio glitch hunting (WASAPI, ~2 GB / min)
    echo   gpu-ui       - GPU/UI performance (DWM/DXGI, ~1 GB / 60s)
    echo   audio-stack  - Full audio subsystem (~1.5 GB / 30s)
    echo.
    echo Duration: Optional. If omitted, runs until Ctrl+C.
    echo.
    echo Examples:
    echo   run_etw_capture.cmd debug-light 30
    echo   run_etw_capture.cmd glitch-hunt
    echo.
    pause
    exit /b 1
)

set PRESET=%~1
set DURATION=%~2

REM Validate preset
if /i not "%PRESET%"=="debug-light" (
    if /i not "%PRESET%"=="glitch-hunt" (
        if /i not "%PRESET%"=="gpu-ui" (
            if /i not "%PRESET%"=="audio-stack" (
                echo.
                echo [ERROR] Invalid preset: %PRESET%
                echo.
                echo Valid presets: debug-light, glitch-hunt, gpu-ui, audio-stack
                echo.
                pause
                exit /b 1
            )
        )
    )
)

REM ==============================================================================
REM Check WPR Availability
REM ==============================================================================

where wpr.exe >nul 2>&1
if %errorLevel% neq 0 (
    echo.
    echo ==============================================================================
    echo   ERROR: Windows Performance Recorder (WPR) Not Found
    echo ==============================================================================
    echo.
    echo WPR is part of the Windows Performance Toolkit.
    echo.
    echo Install from:
    echo   - Windows ADK: https://learn.microsoft.com/en-us/windows-hardware/get-started/adk-install
    echo   - Select "Windows Performance Toolkit" during installation
    echo.
    pause
    exit /b 1
)

REM ==============================================================================
REM Locate WPRP Profile
REM ==============================================================================

REM Get script directory
set SCRIPT_DIR=%~dp0
set WPRP_FILE=%SCRIPT_DIR%zenith_etw.wprp

if not exist "%WPRP_FILE%" (
    echo.
    echo [ERROR] WPRP profile not found: %WPRP_FILE%
    echo.
    echo Please ensure zenith_etw.wprp exists in the same directory as this script.
    echo.
    pause
    exit /b 1
)

REM ==============================================================================
REM Create Output Directory
REM ==============================================================================

REM Get repository root (4 levels up from scripts\windows\perf\etw\)
set REPO_ROOT=%SCRIPT_DIR%..\..\..\..\
pushd "%REPO_ROOT%"
set REPO_ROOT=%CD%
popd

REM Create timestamped directory
for /f "tokens=1-3 delims=/ " %%a in ("%date%") do set DATE_STR=%%c%%a%%b
for /f "tokens=1-3 delims=:. " %%a in ("%time%") do set TIME_STR=%%a%%b%%c

REM Handle single-digit hours (add leading zero)
if "%TIME_STR:~0,1%"==" " set TIME_STR=0%TIME_STR:~1%
set TIMESTAMP=%DATE_STR%_%TIME_STR%

set OUTPUT_DIR=%REPO_ROOT%\logs\etw\%TIMESTAMP%
set ETL_FILE=%OUTPUT_DIR%\session.etl

if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
    echo [INFO] Created output directory: %OUTPUT_DIR%
)

REM ==============================================================================
REM Cancel Any Existing WPR Session
REM ==============================================================================

echo.
echo ==============================================================================
echo   Preparing ETW Capture
echo ==============================================================================
echo.

wpr -status >nul 2>&1
if %errorLevel% equ 0 (
    echo [WARNING] Existing WPR session detected. Canceling...
    wpr -cancel >nul 2>&1
    timeout /t 2 >nul
)

REM ==============================================================================
REM Validate WPRP Profile
REM ==============================================================================

echo [INFO] Validating WPRP profile...
wpr -validate "%WPRP_FILE%" >nul 2>&1
if %errorLevel% neq 0 (
    echo.
    echo [ERROR] WPRP profile validation failed.
    echo.
    echo Running validation with output:
    wpr -validate "%WPRP_FILE%"
    echo.
    pause
    exit /b 1
)
echo [INFO] WPRP profile validated successfully.

REM ==============================================================================
REM Display Capture Settings
REM ==============================================================================

echo.
echo ==============================================================================
echo   ETW Capture Settings
echo ==============================================================================
echo   Preset:       %PRESET%
echo   Profile:      %WPRP_FILE%
echo   Output:       %ETL_FILE%

if "%DURATION%"=="" (
    echo   Duration:     Continuous (Ctrl+C to stop)
) else (
    echo   Duration:     %DURATION% seconds
)

echo ==============================================================================
echo.

REM ==============================================================================
REM Write Provider List
REM ==============================================================================

echo debug-light=%PRESET% > "%OUTPUT_DIR%\providers.txt"
echo Kernel: CpuConfig, Loader, ProcessThread, CSwitch, Dispatcher, SampledProfile, DPC, ISR >> "%OUTPUT_DIR%\providers.txt"

if /i "%PRESET%"=="debug-light" (
    echo User: DWM, DXGI, Win32k >> "%OUTPUT_DIR%\providers.txt"
)
if /i "%PRESET%"=="glitch-hunt" (
    echo User: Audio, AudioSes, WASAPI, AudioKSE, MMCSS, AudioClient >> "%OUTPUT_DIR%\providers.txt"
)
if /i "%PRESET%"=="gpu-ui" (
    echo User: DWM, DXGI, Win32k, DxgKrnl, PerfInfo >> "%OUTPUT_DIR%\providers.txt"
)
if /i "%PRESET%"=="audio-stack" (
    echo Kernel: +DiskIO, +HardFaults, +VirtualAlloc >> "%OUTPUT_DIR%\providers.txt"
    echo User: Audio, AudioSes, WASAPI, AudioKSE, MMCSS, AudioClient, DWM, ProcPower >> "%OUTPUT_DIR%\providers.txt"
)

REM ==============================================================================
REM Start WPR Recording
REM ==============================================================================

echo [INFO] Starting WPR recording...
echo.

wpr -start "%WPRP_FILE%!%PRESET%"

if %errorLevel% neq 0 (
    echo.
    echo ==============================================================================
    echo   ERROR: WPR Failed to Start
    echo ==============================================================================
    echo.
    echo Common Issues:
    echo   - Another WPR session is running (run: wpr -cancel)
    echo   - Insufficient privileges (run as Administrator)
    echo   - WPRP profile syntax error (run: wpr -validate zenith_etw.wprp)
    echo.
    pause
    exit /b 1
)

echo ==============================================================================
echo   Recording in Progress...
echo ==============================================================================
echo.

if "%DURATION%"=="" (
    echo Press Ctrl+C to stop recording.
    echo.
    pause >nul
) else (
    echo Capturing for %DURATION% seconds...
    timeout /t %DURATION% >nul
)

REM ==============================================================================
REM Stop WPR and Save Trace
REM ==============================================================================

echo.
echo ==============================================================================
echo   Stopping Recording...
echo ==============================================================================
echo.

wpr -stop "%ETL_FILE%"

if %errorLevel% neq 0 (
    echo.
    echo [ERROR] Failed to stop recording or save trace.
    echo.
    pause
    exit /b 1
)

REM ==============================================================================
REM Summary
REM ==============================================================================

echo.
echo ==============================================================================
echo   ETW Capture Complete
echo ==============================================================================
echo   Output File:  %ETL_FILE%
echo   Directory:    %OUTPUT_DIR%
echo.
echo   Next Steps:
echo     1. Open Windows Performance Analyzer (WPA)
echo     2. File -^> Open -^> %ETL_FILE%
echo     3. Load starter profile: scripts\windows\perf\etw\wpa_starter_profile.wpaProfile
echo     4. Analyze graphs (see README.md for analysis workflow)
echo.

REM Show file size
for %%A in ("%ETL_FILE%") do set FILE_SIZE=%%~zA
set /a SIZE_MB=!FILE_SIZE! / 1048576
echo   Trace Size:   !SIZE_MB! MB
echo ==============================================================================
echo.

pause
endlocal
