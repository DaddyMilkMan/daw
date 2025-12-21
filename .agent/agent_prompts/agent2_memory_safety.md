# Agent 2 Prompt: Thread Safety + Memory Bugs (Bugs 16-30)

## Objective
Fix bugs 16-30 covering remaining thread safety issues and memory safety problems.

## Bugs to Fix

### Bug 16: Concurrent Modification of `midiSessions_`
**File:** `c:\zenith\daw\apps\desktop\Source\engine\RecordingManager.cpp`
**Fix:** Ensure all `midiSessions_` access is under `sessionLock_`

### Bug 17: Lock-Free FIFO Overflow Ignored
**File:** `c:\zenith\daw\apps\desktop\Source\engine\RecordingManager.cpp` (Line 225)
**Fix:** Add counter for dropped messages and log warning periodically

### Bug 18: Memory Ordering Mismatch
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Clip.cpp` (Lines 216, 273)
**Fix:** Ensure matching acquire/release pairs

### Bug 19: Raw `delete reader` Without Smart Pointer
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Clip.cpp` (Line 183)
**Fix:** Use `std::unique_ptr<juce::AudioFormatReader>` 

### Bug 20: Memory Leak in `RoutingGraph.cpp`
**File:** `c:\zenith\daw\apps\desktop\Source\engine\RoutingGraph.cpp` (Lines 296-314)
**Fix:** Use `juce::DynamicObject::Ptr` instead of raw `new`

### Bug 21: Raw `new` in Synth Voice Creation
**File:** `c:\zenith\daw\apps\desktop\Source\instruments\ZenithPolySynth.cpp` (Lines 180, 364)
**Fix:** Document ownership or use factory pattern

### Bug 22: Leak in `TrackFreeze.cpp`
**File:** `c:\zenith\daw\apps\desktop\Source\engine\TrackFreeze.cpp` (Line 229)
**Fix:** Use `std::make_unique` instead of raw `new`

### Bug 23: Potential Double-Delete
**File:** `c:\zenith\daw\apps\desktop\Source\ui\widgets\SkiaAlertWindow.cpp` (Lines 141, 167)
**Fix:** Use weak pointer or flag to prevent double delete

### Bug 24: Unbounded Buffer Growth
**File:** `c:\zenith\daw\apps\desktop\Source\instruments\ZenithPolySynth.cpp` (Line 375)
**Fix:** Validate buffer size and add bounds check

### Bug 25: Missing Destructor Cleanup
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Track.cpp` (Line 30-31)
**Fix:** Make destructor virtual and ensure derived classes clean up

### Bug 26: AudioSource Ownership Issue
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Clip.cpp` (Line 192)
**Fix:** Restructure to avoid accessing reader after ownership transfer

### Bug 27: Format Manager Registration Leak
**File:** `c:\zenith\daw\apps\desktop\Source\engine\Engine.cpp` (Lines 1538-1539)
**Fix:** Use JUCE's ownership model, document properly

### Bug 28: RecentProjectManager Leak
**File:** `c:\zenith\daw\apps\desktop\Source\engine\RecentProjectManager.cpp` (Line 48)
**Fix:** Use `juce::DynamicObject::Ptr obj = new juce::DynamicObject()`

### Bug 29: Plugin State Memory Block Handling
**File:** `c:\zenith\daw\apps\desktop\Source\engine\TrackPluginState.cpp`
**Fix:** Ensure RAII wrapper usage throughout

### Bug 30: Visualizer Buffer Not Cleared on Prepare
**File:** `c:\zenith\daw\apps\desktop\Source\instruments\ZenithPolySynth.cpp` (Lines 194-195)
**Fix:** Zero-initialize buffer after reset

## Verification
Build with Address Sanitizer (ASan) enabled and run test suite.
