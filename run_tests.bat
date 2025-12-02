@echo off
REM ========================================
REM ZENITH DAW - Test Runner
REM Operation Polish Phase 2
REM ========================================

echo.
echo ╔════════════════════════════════════════╗
echo ║   ZENITH DAW - RUNNING UNIT TESTS      ║
echo ╚════════════════════════════════════════╝
echo.

REM Build tests first
echo [1/2] Building test suite...
cd zenith-core\build
cmake --build . --config Debug --target ZenithDAWTests

if errorlevel 1 (
    echo ✗ Test build failed!
    pause
    exit /b 1
)

echo ✓ Tests built successfully
echo.

REM Run tests
echo [2/2] Running tests...
.\ZenithDAW_artefacts\Debug\ZenithDAWTests.exe

echo.
echo Tests complete!
pause
