@echo off
REM ==============================================================================
REM PresentMon Frame-Time Capture for Zenith DAW (W6)
REM ==============================================================================
REM
REM Usage: run_presentmon_example.cmd
REM
REM Prerequisites:
REM   - Run as Administrator
REM   - PresentMon.exe in PATH or same directory as this script
REM   - ZenithDAW.exe must be running
REM
REM Output: presentmon_logs\zenith_frame_times.csv
REM ==============================================================================

echo.
echo ==============================================================================
echo   PresentMon Frame-Time Capture for Zenith DAW
echo ==============================================================================
echo.

REM Check for Administrator privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] This script requires Administrator privileges.
    echo         Right-click and select "Run as Administrator"
    echo.
    pause
    exit /b 1
)

REM Create output directory if it doesn't exist
if not exist "presentmon_logs" (
    echo [INFO] Creating output directory: presentmon_logs\
    mkdir presentmon_logs
)

REM Check if PresentMon.exe exists
where PresentMon.exe >nul 2>&1
if %errorLevel% neq 0 (
    if not exist "PresentMon.exe" (
        echo [ERROR] PresentMon.exe not found in PATH or current directory.
        echo.
        echo Please download PresentMon from:
        echo   https://github.com/GameTechDev/PresentMon/releases
        echo.
        echo Extract PresentMon.exe to this directory or add to PATH.
        echo.
        pause
        exit /b 1
    )
)

echo [INFO] PresentMon.exe found
echo.

REM Check if ZenithDAW.exe is running
tasklist /FI "IMAGENAME eq ZenithDAW.exe" 2>NUL | find /I /N "ZenithDAW.exe">NUL
if %errorLevel% neq 0 (
    echo [WARNING] ZenithDAW.exe not currently running.
    echo           Start Zenith DAW before capturing metrics.
    echo.
    echo Press any key to continue anyway, or Ctrl+C to exit...
    pause >nul
)

REM Display capture settings
echo ==============================================================================
echo   Capture Settings
echo ==============================================================================
echo   Process:      ZenithDAW.exe
echo   Output File:  presentmon_logs\zenith_frame_times.csv
echo   Timestamp:    QueryPerformanceCounter (high-resolution)
echo   Duration:     Indefinite (Ctrl+C to stop)
echo ==============================================================================
echo.
echo [INFO] Starting capture... Press Ctrl+C to stop
echo.

REM Run PresentMon with recommended flags
REM   --process_name: Capture only Zenith DAW
REM   --output: CSV output path
REM   --no_csv_summary: Skip summary file (only raw frame data)
REM   --qpc_time: Use QueryPerformanceCounter timestamps
REM   --scroll_output: Show live metrics in console

PresentMon.exe --process_name ZenithDAW.exe ^
               --output presentmon_logs\zenith_frame_times.csv ^
               --no_csv_summary ^
               --qpc_time ^
               --scroll_output

REM Check if capture completed successfully
if %errorLevel% equ 0 (
    echo.
    echo ==============================================================================
    echo   Capture Complete
    echo ==============================================================================
    echo   Output: presentmon_logs\zenith_frame_times.csv
    echo.
    echo   Next Steps:
    echo     1. Open CSV in Excel or CapFrameX for analysis
    echo     2. Calculate average FPS: 1000 / AVG(msBetweenPresents)
    echo     3. Check for frame pacing issues (variance in msBetweenPresents)
    echo ==============================================================================
) else (
    echo.
    echo ==============================================================================
    echo   Capture Failed or Interrupted
    echo ==============================================================================
    echo   Error Code: %errorLevel%
    echo.
    echo   Common Issues:
    echo     - ZenithDAW.exe not running
    echo     - Insufficient permissions (Run as Administrator)
    echo     - Antivirus blocking ETW tracing
    echo ==============================================================================
)

echo.
pause
