# Agent 3 Prompt: Memory + Logic Bugs (Bugs 31-45)

## Objective
Fix bugs 31-45 covering remaining memory issues and logic errors.

## Bugs to Fix

### Bug 31: Clip Audio Buffer Reallocation
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Clip.cpp` (Lines 169-170)
**Fix:** Guard `setSize()` with lock or ensure only called from message thread

### Bug 32: Missing `releaseResources()` Call
**File:** `c:\zenith\daw\apps\desktop\Source\ai\AIMasteringAgent.cpp`
**Fix:** Add destructor that calls reset on DSP components

### Bug 33: Shared Pointer Circular Reference
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Track.h`
**Fix:** Use `std::weak_ptr` for back-references

### Bug 34: Duplicate DBG Statements
**File:** `c:\zenith\daw\apps\desktop\Source\engine\AudioRenderer.cpp` (Lines 57-62)
**Fix:** Remove duplicate `DBG()` call

### Bug 35: Duplicate Update Metering Comments
**File:** `c:\zenith\daw\apps\desktop\Source\engine\AudioRenderer.cpp` (Lines 247-250)
**Fix:** Remove duplicate comment

### Bug 36: Duplicate Skia Includes
**File:** `c:\zenith\daw\apps\desktop\Source\ui\widgets\ZenithButton.h` (Lines 25-37)
**Fix:** Remove the second duplicate `#ifdef ZENITH_USE_SKIA` block

### Bug 37: Possible Division by Zero
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Clip.cpp` (Lines 663, 670)
**Fix:** Add explicit guard: `if (fadeIn > 0)`

### Bug 38: Integer Overflow in Sample Position
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Clip.cpp` (Line 77)
**Fix:** Add upper bound check: `juce::jlimit(int64_t(0), MAX_SAMPLES, position)`

### Bug 39: Modulo by Zero Risk
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Clip.cpp` (Lines 516, 549-551)
**Fix:** Check `sourceBuffer->getNumSamples() > 0` before modulo

### Bug 40: Unreachable `default` in Track Factory
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Track.cpp` (Line 21)
**Fix:** Add `jassertfalse` or throw exception for unknown types

### Bug 41: Ignored Return Value
**File:** Various files
**Fix:** Consistently check return values of file operations

### Bug 42: Wrong Parameter Order Possible
**File:** `c:\zenith\daw\apps\desktop\Source\engine\AudioRenderer.cpp`
**Fix:** Consider strong typing or named parameters

### Bug 43: Fallthrough in `processMidiClip`
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Clip.cpp` (Line 640)
**Fix:** Add comment explaining why params are unused or remove method

### Bug 44: Empty Method Body with Assert
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Track.cpp` (Lines 179-182)
**Fix:** Either implement properly or make pure virtual

### Bug 45: Inconsistent Looping Logic
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Clip.cpp` (Lines 549-555)
**Fix:** Review and fix boundary conditions

## Verification
Run unit tests to verify logic changes don't break functionality.
