# Agent 5 Prompt: Code Quality + RT Safety (Bugs 61-75)

## Objective
Fix bugs 61-75 covering code quality and real-time audio safety issues.

## Bugs to Fix

### Bug 61: Long Methods
**File:** `c:\zenith\daw\apps\desktop\Source\engine\AudioRenderer.cpp`
**Fix:** Split `renderAudioGraph` into smaller helper methods

### Bug 62: Excessive Nesting
**File:** `c:\zenith\daw\apps\desktop\Source\ui\arranger\ArrangerComponent.cpp`
**Fix:** Use early returns and extract helper functions

### Bug 63: Missing Documentation
**File:** Many implementation files
**Fix:** Add Doxygen comments to public APIs

### Bug 64: Inconsistent Error Handling
**File:** Throughout codebase
**Fix:** Standardize on Result<T, E> pattern or exceptions

### Bug 65: Redundant Null Checks
**File:** `c:\zenith\daw\apps\desktop\Source\ai\AIMasteringAgent.cpp` (Lines 440-462)
**Fix:** Restructure to single validation block

### Bug 66: Copy-Paste Code Smell
**File:** `c:\zenith\daw\apps\desktop\Source\instruments\RegisterBuiltInInstruments.cpp`
**Fix:** Extract common `dynamic_cast` pattern into helper function

### Bug 67: Magic Number for Max Voices
**File:** `c:\zenith\daw\apps\desktop\Source\instruments\ZenithPolySynth.cpp`
**Fix:** Create named constant `kDefaultMaxVoices`

### Bug 68: Inconsistent Const Correctness
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Track.cpp`, `Clip.cpp`
**Fix:** Audit and add `const` to all pure getters

### Bug 69: Memory Allocation in Audio Callback
**File:** `c:\zenith\daw\apps\desktop\Source\engine\AudioRenderer.cpp` (Line 124)
**Fix:** Pre-allocate vector to max size in `prepare()`

### Bug 70: String Construction in Audio Path
**File:** Multiple files
**Fix:** Remove or guard DBG calls with `#if JUCE_DEBUG`

### Bug 71: Lock in Audio Callback
**File:** Various Track operations
**Fix:** Use lock-free patterns (atomics, FIFOs) instead

### Bug 72: File I/O in Near-Audio Code
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Clip.cpp`
**Fix:** Add jassert for message thread

### Bug 73: Exception Risk in Audio Path
**File:** `c:\zenith\daw\apps\desktop\Source\engine\AudioRenderer.cpp`
**Fix:** Use fixed-size containers, avoid operations that throw

### Bug 74: Virtual Function Calls in Audio Loop
**File:** `c:\zenith\daw\apps\desktop\Source\engine\AudioRenderer.cpp`
**Fix:** Cache function pointers or use CRTP pattern

### Bug 75: Dynamic Cast in Audio Path
**File:** `c:\zenith\daw\apps\desktop\Source\instruments\ZenithPolySynth.cpp`
**Fix:** Use static_cast with type guarantee or store typed pointers

## Verification
Profile audio callback to ensure no RT violations remain.
