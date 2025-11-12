@echo off
REM ==============================================================================
REM Open Latest Crashpad Dump with WinDbg (W8)
REM ==============================================================================
REM
REM This script locates the newest .dmp file in Crashpad's database and
REM launches WinDbg (if installed) for analysis.
REM
REM Prerequisites:
REM   - WinDbg installed (Microsoft Store or Windows SDK)
REM   - At least one .dmp file in crashpad_db\completed\
REM
REM ==============================================================================

setlocal enabledelayedexpansion

echo.
echo ==============================================================================
echo   Crashpad Dump Analyzer - Launch Latest Dump
echo ==============================================================================
echo.

REM ==============================================================================
REM Locate Crashpad Database
REM ==============================================================================

set CRASHPAD_DB=%APPDATA%\ZenithDAW\crashpad_db\completed

if not exist "%CRASHPAD_DB%" (
    echo [ERROR] Crashpad database not found.
    echo.
    echo Expected location: %CRASHPAD_DB%
    echo.
    echo Possible causes:
    echo   - Crash reporting not enabled
    echo   - No crashes have occurred yet
    echo   - Database directory was deleted
    echo.
    pause
    exit /b 1
)

echo [INFO] Crashpad database: %CRASHPAD_DB%

REM ==============================================================================
REM Find Latest .dmp File
REM ==============================================================================

echo [INFO] Searching for .dmp files...
echo.

REM Get newest .dmp file (sorted by modification time, descending)
set LATEST_DUMP=
for /f "delims=" %%A in ('dir /b /o-d /a-d "%CRASHPAD_DB%\*.dmp" 2^>nul') do (
    set LATEST_DUMP=%CRASHPAD_DB%\%%A
    goto :found_dump
)

:no_dumps
echo [ERROR] No .dmp files found in %CRASHPAD_DB%
echo.
echo Trigger a test crash to generate a dump:
echo   1. Launch Zenith DAW (Debug build)
echo   2. Menu: Help -^> Diagnostics -^> Trigger Test Crash
echo.
pause
exit /b 1

:found_dump
echo [OK] Found latest dump: %LATEST_DUMP%

REM Show dump file info
for %%A in ("%LATEST_DUMP%") do (
    echo [INFO] File size: %%~zA bytes
    echo [INFO] Modified:  %%~tA
)
echo.

REM ==============================================================================
REM Locate WinDbg
REM ==============================================================================

echo [INFO] Locating WinDbg...

set WINDBG_PATH=

REM Check for WinDbg Preview (Microsoft Store version)
for /f "tokens=*" %%A in ('where /R "%LOCALAPPDATA%\Microsoft\WindowsApps" WinDbgX.exe 2^>nul') do (
    set WINDBG_PATH=%%A
    goto :windbg_found
)

REM Check for WinDbg Classic (Windows SDK)
for %%D in (
    "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\windbg.exe"
    "C:\Program Files\Windows Kits\10\Debuggers\x64\windbg.exe"
) do (
    if exist %%D (
        set WINDBG_PATH=%%~D
        goto :windbg_found
    )
)

REM Check PATH
where windbg.exe >nul 2>&1
if %errorLevel% equ 0 (
    for /f "tokens=*" %%A in ('where windbg.exe') do (
        set WINDBG_PATH=%%A
        goto :windbg_found
    )
)

where WinDbgX.exe >nul 2>&1
if %errorLevel% equ 0 (
    for /f "tokens=*" %%A in ('where WinDbgX.exe') do (
        set WINDBG_PATH=%%A
        goto :windbg_found
    )
)

:windbg_not_found
echo [ERROR] WinDbg not found.
echo.
echo Install WinDbg:
echo   Option 1: Microsoft Store -^> Search "WinDbg Preview"
echo   Option 2: Windows SDK -^> Select "Debugging Tools for Windows"
echo             https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/
echo.
echo Manual dump analysis:
echo   1. Install WinDbg
echo   2. Open: %LATEST_DUMP%
echo   3. Run: !analyze -v
echo.
pause
exit /b 1

:windbg_found
echo [OK] Found WinDbg: %WINDBG_PATH%

REM ==============================================================================
REM Launch WinDbg with Dump
REM ==============================================================================

echo.
echo ==============================================================================
echo   Launching WinDbg...
echo ==============================================================================
echo.
echo [INFO] Opening dump: %LATEST_DUMP%
echo.
echo Recommended WinDbg commands after load:
echo   !analyze -v     - Analyze crash (shows faulting code, call stack)
echo   k               - Display call stack
echo   lm              - List loaded modules
echo   dv              - Display local variables
echo   .sympath SRV*C:\Symbols*https://msdl.microsoft.com/download/symbols
echo                   - Configure Microsoft symbol server
echo.

REM Launch WinDbg in a new window (start command)
start "" "%WINDBG_PATH%" -z "%LATEST_DUMP%"

if %errorLevel% neq 0 (
    echo [ERROR] Failed to launch WinDbg
    pause
    exit /b 1
)

echo [OK] WinDbg launched successfully
echo.
echo When WinDbg opens:
echo   1. Wait for symbols to load (~1-2 minutes first time)
echo   2. Run: !analyze -v
echo   3. Review faulting module and call stack
echo.
echo Tip: Configure symbol path for better stack traces:
echo   File -^> Settings -^> Debugging Settings -^> Symbol Path
echo   Add: SRV*C:\Symbols*https://msdl.microsoft.com/download/symbols
echo.

REM Keep window open briefly
timeout /t 5 >nul

endlocal
