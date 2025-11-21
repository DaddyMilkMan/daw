@echo off
REM verify_build.bat - Comprehensive build verification
echo ========================================
echo Zenith DAW - Build Verification Suite
echo ========================================
echo.

REM 1. Check if build completed successfully
if not exist build\ZenithDAW_artefacts\Debug\ZenithDAW.exe (
    echo [FAIL] Main executable not found!
    echo Build may have failed or not completed.
    pause
    exit /b 1
)

echo [PASS] Main executable exists

REM 2. Run synth headless test
echo.
echo [TEST 1/4] Running synth headless test...
if exist build\SynthHeadlessTest_artefacts\Debug\SynthHeadlessTest.exe (
    build\SynthHeadlessTest_artefacts\Debug\SynthHeadlessTest.exe
    if %ERRORLEVEL% EQU 0 (
        echo [PASS] Synth headless test succeeded
    ) else (
        echo [FAIL] Synth headless test failed
        set TESTS_FAILED=1
    )
) else (
    echo [SKIP] SynthHeadlessTest.exe not found
)

REM 3. Run preset regression tests
echo.
echo [TEST 2/4] Running preset regression tests...
if exist build\PresetRegressionTests_artefacts\Debug\PresetRegressionTests.exe (
    build\PresetRegressionTests_artefacts\Debug\PresetRegressionTests.exe
    if %ERRORLEVEL% EQU 0 (
        echo [PASS] Preset regression tests succeeded
    ) else (
        echo [FAIL] Preset regression tests failed
        set TESTS_FAILED=1
    )
) else (
    echo [SKIP] PresetRegressionTests.exe not found
)

REM 4. Run instrument validation tests
echo.
echo [TEST 3/4] Running instrument validation tests...
if exist build\InstrumentValidationTests_artefacts\Debug\InstrumentValidationTests.exe (
    build\InstrumentValidationTests_artefacts\Debug\InstrumentValidationTests.exe
    if %ERRORLEVEL% EQU 0 (
        echo [PASS] Instrument validation tests succeeded
    ) else (
        echo [FAIL] Instrument validation tests failed
        set TESTS_FAILED=1
    )
) else (
    echo [SKIP] InstrumentValidationTests.exe not found
)

REM 5. Verify Skia integration
echo.
echo [TEST 4/4] Verifying Skia integration...
if exist build\CMakeCache.txt (
    findstr /C:"ZENITH_ENABLE_SKIA" build\CMakeCache.txt >nul
    if %ERRORLEVEL% EQU 0 (
        echo [PASS] Skia integration enabled in build
    ) else (
        echo [INFO] Skia integration not enabled
    )
) else (
    echo [SKIP] CMakeCache.txt not found
)

echo.
echo ========================================
if defined TESTS_FAILED (
    echo VERIFICATION: SOME TESTS FAILED
    echo Please review the output above.
) else (
    echo VERIFICATION: ALL TESTS PASSED!
)
echo ========================================
echo.
pause
