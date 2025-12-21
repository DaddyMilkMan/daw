# Agent 4 Prompt: Logic + Code Quality Bugs (Bugs 46-60)

## Objective
Fix bugs 46-60 covering remaining logic errors and code quality issues.

## Bugs to Fix

### Bug 46: Magic Numbers
**File:** `c:\zenith\daw\apps\desktop\Source\engine\AudioRenderer.cpp` (Line 245)
**Fix:** Create constant `kDACBitDepth = 24` and document

### Bug 47: Incorrect LUFS Target Constant
**File:** `c:\zenith\daw\apps\desktop\Source\ai\AIMasteringAgent.cpp`
**Fix:** Make configurable or document the choice

### Bug 48: Off-by-One in Track Index
**File:** `c:\zenith\daw\apps\desktop\Source\ui\arranger\ArrangerComponent.cpp`
**Fix:** Add bounds checking in `yToTrackIndex`

### Bug 49: Stale Layout Cache
**File:** `c:\zenith\daw\apps\desktop\Source\ui\widgets\ZenithButton.h` (Lines 181-182)
**Fix:** Ensure dirty flags reset in `calculateLayout()`

### Bug 50: Missing Tempo Validation
**File:** `c:\zenith\daw\apps\desktop\Source\engine\RecordingManager.cpp` (Lines 346-347)
**Fix:** Add guard: `if (tempo <= 0.0) tempo = 120.0;`

### Bug 51: Incorrect Range Clamp
**File:** `c:\zenith\daw\apps\desktop\Source\ai\AIMasteringAgent.cpp` (Line 295)
**Fix:** Add comment explaining negative value range

### Bug 52: TrackIndex Out of Bounds Silent Skip
**File:** `c:\zenith\daw\apps\desktop\Source\engine\AudioRenderer.cpp` (Lines 172-173)
**Fix:** Add `DBG` warning when skip occurs

### Bug 53: Unused Variable Warning
**File:** `c:\zenith\daw\apps\desktop\Source\instruments\ZenithPolySynth.cpp` (Line 189)
**Fix:** Use parameter for validation or document why unused

### Bug 54: TODO Comments - Arranger
**File:** `c:\zenith\daw\apps\desktop\Source\ui\arranger\ArrangerComponent.cpp` (Line 290)
**Fix:** Implement Mute/Solo/Rec sync from ValueTree

### Bug 55: TODO Comments - Track Component
**File:** `c:\zenith\daw\apps\desktop\Source\ui\arranger\ArrangerTrackComponent.cpp` (Line 320)
**Fix:** Implement sync to ValueTree

### Bug 56: TODO in CMakeLists
**File:** `c:\zenith\daw\apps\desktop\Source\engine\CMakeLists.txt` (Line 38)
**Fix:** Complete ProjectEngineBridge integration and remove legacy files

### Bug 57: Inconsistent Naming Convention
**File:** Throughout codebase
**Fix:** Create and apply naming convention document

### Bug 58: Empty Else Branches
**File:** Various files
**Fix:** Remove or add meaningful code to empty blocks

### Bug 59: Unused Parameters
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Track.cpp` (Line 179)
**Fix:** Use `[[maybe_unused]]` attribute or remove parameter

### Bug 60: Hardcoded String Literals
**File:** Multiple files
**Fix:** Create constants file for common strings

## Verification
Build clean with `-Wall -Wextra -Wpedantic` and fix any new warnings.
