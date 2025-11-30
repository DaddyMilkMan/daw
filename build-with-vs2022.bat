@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Zenith DAW Build Script (VS2026 with patches)
echo ========================================
echo.

REM Set up Visual Studio 2026 environment
echo [1/5] Setting up Visual Studio 2026 environment...
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 (
    echo ERROR: Failed to set up VS2022 environment
    exit /b 1
)
echo VS2022 environment configured successfully
echo.

REM Clean and create build directory
echo [2/5] Cleaning build directory...
cd /d C:\zenith\daw
rd /s /q build 2>nul
mkdir build
cd build

REM Configure with CMake using explicit VS2022 compiler
echo [3/5] Running CMake configuration...
cmake .. -G Ninja ^
    -DCMAKE_CXX_COMPILER=cl.exe ^
    -DCMAKE_C_COMPILER=cl.exe ^
    -DZENITH_ENABLE_SKIA=OFF ^
    -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    exit /b 1
)
echo CMake configuration successful
echo.

REM Apply JUCE patches for VS2026 compatibility
echo [4/5] Applying JUCE compatibility patches...

REM Patch 1: juce_AccessibilityElement_windows.cpp (line 425)
powershell -Command "(Get-Content '_deps\juce-src\modules\juce_gui_basics\native\accessibility\juce_AccessibilityElement_windows.cpp') -replace '\(pointer_sized_int\) accessibilityHandler\.getComponent\(\)\.getWindowHandle\(\)', '(INT_PTR) accessibilityHandler.getComponent().getWindowHandle()' | Set-Content '_deps\juce-src\modules\juce_gui_basics\native\accessibility\juce_AccessibilityElement_windows.cpp'"

REM Patch 2: juce_RelativeCoordinatePositioner.cpp (line 73)
powershell -Command "(Get-Content '_deps\juce-src\modules\juce_gui_basics\positioning\juce_RelativeCoordinatePositioner.cpp') -replace 'String::toHexString \(\(pointer_sized_int\) \(void\*\) &component\) \+ \"m\"', 'String::toHexString ((INT_PTR) (void*) &component) + \"m\"' | Set-Content '_deps\juce-src\modules\juce_gui_basics\positioning\juce_RelativeCoordinatePositioner.cpp'"

REM Patch 3: juce_RelativeCoordinatePositioner.cpp (line 156)
powershell -Command "(Get-Content '_deps\juce-src\modules\juce_gui_basics\positioning\juce_RelativeCoordinatePositioner.cpp') -replace 'String::toHexString \(\(pointer_sized_int\) \(void\*\) &component\);', 'String::toHexString ((INT_PTR) (void*) &component);' | Set-Content '_deps\juce-src\modules\juce_gui_basics\positioning\juce_RelativeCoordinatePositioner.cpp'"

REM Patch 4: juce_HashMap.h (line 61)
powershell -Command "(Get-Content '_deps\juce-src\modules\juce_core\containers\juce_HashMap.h') -replace 'return generateHash \(\(uint64\) \(pointer_sized_uint\) key, upperLimit\);', 'return generateHash ((uint64) (uintptr_t) key, upperLimit);' | Set-Content '_deps\juce-src\modules\juce_core\containers\juce_HashMap.h'"

REM Patch 5: Add missing CaretPosition enum to juce_UIATextProvider_windows.h
powershell -Command "$content = Get-Content '_deps\juce-src\modules\juce_gui_basics\native\accessibility\juce_UIATextProvider_windows.h' -Raw; if ($content -notmatch 'CaretPosition_Unknown') { $content = $content -replace '(#pragma once[\r\n]+)', \"`$1`r`n// VS2026 compatibility: Define missing CaretPosition enums if not in SDK`r`n#ifndef CaretPosition_Unknown`r`nenum CaretPosition`r`n{`r`n    CaretPosition_Unknown = 0,`r`n    CaretPosition_EndOfLine = 1,`r`n    CaretPosition_BeginningOfLine = 2`r`n};`r`n#endif`r`n`r`n\"; Set-Content '_deps\juce-src\modules\juce_gui_basics\native\accessibility\juce_UIATextProvider_windows.h' -Value $content -NoNewline }"

echo Patches applied successfully
echo.

REM Build the project
echo [5/5] Building ZenithDAW...
ninja ZenithDAW

if errorlevel 1 (
    echo.
    echo ========================================
    echo BUILD FAILED
    echo ========================================
    exit /b 1
)

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.
echo Checking for executable...
if exist zenith-core\ZenithDAW.exe (
    echo ZenithDAW.exe created successfully:
    dir zenith-core\ZenithDAW.exe
) else (
    echo Warning: Executable not found in expected location
    echo Searching...
    for /r . %%F in (*.exe) do echo Found: %%F
)

echo.
echo Build complete!
