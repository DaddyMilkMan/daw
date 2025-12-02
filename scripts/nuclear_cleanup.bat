@echo off
REM ========================================
REM OPERATION POLISH - Phase 1: Nuclear Cleanup
REM Victor "The Cleaner" Koskov
REM ========================================

echo.
echo ╔════════════════════════════════════════╗
echo ║  OPERATION POLISH - NUCLEAR CLEANUP    ║
echo ║  Victor "The Cleaner" Koskov          ║
echo ╚════════════════════════════════════════╝
echo.

REM Phase 1.1: Delete redundant batch files
echo [1/6] Purging redundant batch files...
del /Q auto-build.bat 2>nul
del /Q build-attempt.bat 2>nul
del /Q build-baseline.bat 2>nul
del /Q build-fixed.bat 2>nul
del /Q build-juce7.bat 2>nul
del /Q build-nmake.bat 2>nul
del /Q build-no-skia.bat 2>nul
del /Q check-build-status.bat 2>nul
del /Q check-build.bat 2>nul
del /Q check-errors.bat 2>nul
del /Q clean-rebuild-skia.bat 2>nul
del /Q clean-rebuild.bat 2>nul
del /Q compile_test.bat 2>nul
del /Q continue-from-previous.bat 2>nul
del /Q debug_launch.bat 2>nul
del /Q final-clean-build.bat 2>nul
del /Q final-complete-build.bat 2>nul
del /Q final-rebuild.bat 2>nul
del /Q final_build.bat 2>nul
del /Q find-errors.bat 2>nul
del /Q fresh-build.bat 2>nul
del /Q grok-build.bat 2>nul
del /Q msbuild.bat 2>nul
del /Q quick-error-check.bat 2>nul
del /Q rebuild-all-fixes.bat 2>nul
del /Q rebuild-check.bat 2>nul
del /Q rebuild-debuglog-fix.bat 2>nul
del /Q rebuild-panels.bat 2>nul
del /Q rebuild-skia.bat 2>nul
del /Q rebuild.bat 2>nul
del /Q reconfig-and-build.bat 2>nul
del /Q session19-build.bat 2>nul
del /Q short-path-build.bat 2>nul
del /Q status-check.bat 2>nul
del /Q temp-cmake.bat 2>nul
del /Q test-p0-fixes.bat 2>nul
del /Q verify-skia-fixes.bat 2>nul
del /Q verify_build.bat 2>nul
del /Q verify_skia_build.bat 2>nul
del /Q vs-build.bat 2>nul
del /Q build-with-vs2022.bat 2>nul

REM Phase 1.2: Delete all log files
echo [2/6] Purging log files...
del /Q *.log 2>nul
del /Q *.txt 2>nul
del /Q zenith-core\*.log 2>nul
del /Q zenith-core\*.txt 2>nul
del /Q zenith-core\build_log_*.txt 2>nul

REM Phase 1.3: Delete compiled binaries
echo [3/6] Removing compiled binaries...
del /Q test.exe 2>nul
del /Q test_env.exe 2>nul
del /Q test.obj 2>nul
del /Q test_env.obj 2>nul

REM Phase 1.4: Delete temp directories
echo [4/6] Removing temp_ai_setup...
if exist temp_ai_setup rmdir /S /Q temp_ai_setup

REM Phase 1.5: Delete FIXED files from root
echo [5/6] Removing *_FIXED files...
del /Q *_FIXED.* 2>nul

REM Phase 1.6: Delete obsolete markdown clutter
echo [6/6] Archiving redundant documentation...
if not exist docs\archive mkdir docs\archive
move /Y PRODUCER_FEEDBACK_*.md docs\archive\ 2>nul
move /Y TEAM_*.md docs\archive\ 2>nul
move /Y ROUNDTABLE_*.md docs\archive\ 2>nul
move /Y *_COMPLETE.md docs\archive\ 2>nul
move /Y *_SUMMARY.md docs\archive\ 2>nul
move /Y *_FIXES_*.md docs\archive\ 2>nul

echo.
echo ✓ Cleanup complete!
echo ✓ Repository size reduced significantly
echo ✓ Run 'git status' to see changes
echo.
pause
