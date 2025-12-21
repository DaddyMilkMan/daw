# Agent 1 Prompt: Thread Safety Bugs (Bugs 1-15)

## Objective
Fix the first 15 thread safety and race condition bugs in the Zenith DAW codebase.

## Context
You are fixing bugs identified in a comprehensive codebase audit. Focus on thread safety, atomic operations, and lock ordering issues.

## Bugs to Fix

### Bug 1: Vector Operations in Audio Thread
**File:** `c:\zenith\daw\apps\desktop\Source\engine\AudioRenderer.cpp` (Lines 123-124)
**Problem:** `std::vector::clear()` and `push_back()` allocate memory on audio thread
**Fix:** Pre-allocate the vector in `prepare()` and use a fixed-size array or reuse without clearing

### Bug 2: Lock After Atomic Load - Inconsistent Ordering
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Clip.cpp` (Lines 420-421)
**Problem:** Atomic load before lock acquisition creates race
**Fix:** Move atomic load inside the lock scope

### Bug 3: Unsafe `delete this` Pattern
**File:** `c:\zenith\daw\apps\desktop\Source\ui\piano-roll\PianoRollComponent.h` (Line 1331)
**Problem:** Self-deletion causes use-after-free
**Fix:** Use `juce::MessageManager::callAsync` to schedule deletion, or use weak references

### Bug 4: Potential Deadlock in Nested Locks
**File:** `c:\zenith\daw\apps\desktop\Source\engine\RecordingManager.cpp` (Lines 241-242, 262-263)
**Problem:** Lock acquired multiple times in loop
**Fix:** Acquire lock once before the loop

### Bug 5: Missing Synchronization for `currentBpm_`
**File:** `c:\zenith\daw\apps\desktop\Source\instruments\ZenithPolySynth.cpp` (Lines 213-217)
**Problem:** Non-atomic read/write from audio thread
**Fix:** Make `currentBpm_` an `std::atomic<double>`

### Bug 6: Non-Atomic Access to Shared State
**File:** `ZenithPolySynthProcessor::globalModMatrix_`
**Problem:** Matrix accessed from UI and audio threads without sync
**Fix:** Use atomic wrapper or juce::SpinLock for modulation matrix access

### Bug 7: Race Condition in `isConfigured_` Access
**File:** `c:\zenith\daw\apps\desktop\Source\ai\AIMasteringAgent.cpp` (Lines 421-425)
**Problem:** Atomic store inside lock but read without lock
**Fix:** Ensure all reads use same synchronization as writes

### Bug 8: Potential Data Race in Voice Count Update
**File:** `c:\zenith\daw\apps\desktop\Source\instruments\ZenithPolySynth.cpp` (Lines 358-367)
**Problem:** Voice modification without audio thread sync
**Fix:** Use lock-free message passing or suspend audio during voice changes

### Bug 9: Unsafe Shared Pointer in Audio Thread
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Clip.cpp` (Line 487-488)
**Problem:** Shared_ptr refcount operations may allocate
**Fix:** Cache raw pointer locally in audio callback

### Bug 10: Missing Memory Barrier
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Track.cpp` (Line 93)
**Problem:** Only release ordering, needs corresponding acquire
**Fix:** Add acquire ordering on consumer side

### Bug 11: Thread-Unsafe Font Manager Singleton
**File:** `c:\zenith\daw\apps\desktop\Source\ui\design-system\FontManager.cpp`
**Problem:** Static initialization race
**Fix:** Use Meyer's singleton pattern with C++11 thread-safe initialization

### Bug 12: Race in PresetGeneticistAgent Population Access
**File:** `c:\zenith\daw\apps\desktop\Source\ai\PresetGeneticistAgent.cpp`
**Problem:** Some population reads occur without lock
**Fix:** Audit all population access and add locks

### Bug 13: Unsynchronized Access to `masterPlugins`
**File:** `c:\zenith\daw\apps\desktop\Source\engine\AudioRenderer.cpp`
**Problem:** Vector could be modified while iterating
**Fix:** Use RCU pattern or make copy for audio thread

### Bug 14: Timer Callback Race
**File:** `c:\zenith\daw\apps\desktop\Source\engine\ProjectState.cpp` (Lines 189-209)
**Problem:** `timerCallback` accesses state modified elsewhere
**Fix:** Use message thread assertions and proper locking

### Bug 15: Unsafe `sendChangeMessage()` from Any Thread
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Track.cpp`
**Problem:** `sendChangeMessage()` called from non-message threads
**Fix:** Use `juce::MessageManager::callAsync` wrapper

## Verification
After fixes, run the DAW and verify no audio glitches or crashes occur during normal operation.
