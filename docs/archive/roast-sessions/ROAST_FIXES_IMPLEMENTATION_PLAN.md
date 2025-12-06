# Critical Roast Fixes - Implementation Plan

## Executive Summary
This document outlines the systematic fixes for 10 critical technical flaws identified in the Zenith DAW codebase. These issues span thread safety, performance, and architectural concerns.

---

## Roast #1: Use-After-Free Time Bomb in Track Management
**Severity**: CRITICAL  
**Location**: `Engine.h` (line 706-720), `Track.h` (line 264-284)

### Problem
Both `Engine` and `Track` use snapshot patterns with raw pointers. If a track/clip is deleted on the Main Thread, the Audio Thread may access freed memory through old snapshots.

### Current Implementation
```cpp
struct TrackSnapshot {
    std::vector<Track*> tracks; // Raw pointers (non-owning)
}
std::shared_ptr<const TrackSnapshot> tracksSnapshot_;
```

### Fix
Replace raw pointers with `std::shared_ptr` to extend object lifetime:
```cpp
struct TrackSnapshot {
    std::vector<std::shared_ptr<Track>> tracks;
}
```

### Files to Modify
1. `include/Engine.h` - Update `TrackSnapshot` struct
2. `Source/engine/Engine.cpp` - Update snapshot creation and usage
3. `Source/engine/Track.h` - Update `ClipSnapshot` struct  
4. `Source/engine/Track.cpp` - Update clip snapshot creation

---

## Roast #2: Thread Safety Violation in Plugin Chain
**Severity**: CRITICAL  
**Location**: `Track.h` (line 247), `Track.cpp` `processPluginChain()`

### Problem
`Track::processPluginChain` reads `std::vector<unique_ptr<Plugin>> plugins` without synchronization while Main Thread can modify it via `push_back`, causing UB if reallocation occurs.

### Current Implementation
```cpp
// Track.h
std::vector<std::unique_ptr<AudioPlugin Instance>> plugins;
juce::CriticalSection pluginLock; // Only for add/remove, not audio thread
```

### Fix  
Use atomic snapshot pattern for plugins:
```cpp
struct PluginSnapshot {
    std::vector<std::shared_ptr<AudioPluginInstance>> plugins;
};
std::shared_ptr<const PluginSnapshot> pluginSnapshot_;
void updatePluginSnapshot(); // Called after add/remove
```

### Files to Modify
1. `Source/engine/Track.h` - Add `PluginSnapshot` struct
2. `Source/engine/Track.cpp` - Update `addPlugin`, `removePlugin`, `processPluginChain`

---

## Roast #3: "Nuke and Pave" UI Updates
**Severity**: HIGH  
**Location**: `MixerComponent.cpp` (line 302-328)

### Problem
`rebuildTrackStrips()` destroys and recreates ALL track strips when a single track is added/removed, causing UI stuttering on large projects.

### Current Implementation
```cpp
void valueTreeChildAdded(...) {
    rebuildTrackStrips(); // O(N) destroy + create
}
```

### Fix
Implement incremental updates:
```cpp
void valueTreeChildAdded(ValueTree& parent, ValueTree& child) {
    if (parent.getType() == ID_TRACKS) {
        auto strip = createTrackStrip(child);
        trackStrips.push_back(std::move(strip));
        resized();
    }
}

void valueTreeChildRemoved(ValueTree& parent, ValueTree& child, int index) {
if (parent.getType() == ID_TRACKS) {
        trackStrips.erase(trackStrips.begin() + index);
        resized();
    }
}
```

### Files to Modify
1. `Source/ui/MixerComponent.cpp` - Replace rebuild with incremental updates

---

## Roast #4: Blocking I/O on Record Start
**Severity**: MEDIUM  
**Location**: `Engine.cpp` `record()` method

### Problem
`record()` calls `openedOk()` and `createWriterFor()` synchronously on Message Thread, potentially freezing UI.

### Fix
Prepare recording files asynchronously:
```cpp
void record() {
    // Start async file preparation task
    std::async(std::launch::async, [this]() {
        prepareRecordingFiles();
        isRecording_.store(true); // Enable only after files ready
    });
}
```

### Files to Modify
1. `Source/engine/Engine.cpp` - Add async recording preparation

---

## Roast #5: Silent Audio Dropouts
**Severity**: MEDIUM  
**Location**: `Engine.cpp` `renderBlock()` method

### Problem
```cpp
if (trackBuffer.getNumSamples() < numSamples) { continue; } // Silent failure!
```

### Fix
Add debug assertions and logging:
```cpp
if (trackBuffer.getNumSamples() < numSamples) {
    jassertfalse; // Break in debug builds
    DBG("CRITICAL: Track buffer size mismatch! Expected " + 
        String(numSamples) + ", got " + String(trackBuffer.getNumSamples()));
    continue;
}
```

### Files to Modify
1. `Source/engine/Engine.cpp` - Add assertions to `renderBlock()`, `processAudio()`

---

## Roast #6: O(N²) MIDI Note Insertion
**Severity**: HIGH  
**Location**: `ProjectState.cpp` `addNote()` method

### Problem
Linear insertion sort on every note add = O(N²) for bulk imports.

### Fix
Add bulk operation mode:
```cpp
void beginBulkNoteInsert() { suspendNoteSorting_ = true; }
void endBulkNoteInsert() {
    suspendNoteSorting_ = false;
    sortNotes(); // Single O(N log N) sort
}
```

### Files to Modify
1. `Source/engine/ProjectState.h` - Add bulk insert methods
2. `Source/engine/ProjectState.cpp` - Implement deferred sorting

---

## Roast #7: Toy Compressor Implementation
**Severity**: MEDIUM  
**Location**: `MixerChannel.cpp` compressor DSP

### Problem
- Simple peak envelope (no RMS)
- No lookahead (transients clip)
- No oversampling (aliasing on fast attack)

### Fix
Implement proper compressor:
1. RMS detection with configurable window
2. 5ms lookahead buffer
3. Smooth gain reduction (no zipper noise)

### Files to Modify
1. `Source/engine/MixerChannel.h` - Add RMS calculator, lookahead buffer
2. `Source/engine/MixerChannel.cpp` - Rewrite compressor algorithm

---

## Roast #8: Global ID Scanning
**Severity**: LOW  
**Location**: `ProjectState.cpp` `rebuildIdCounter()`

### Problem
Recursive tree scan on project load just to find max ID.

### Fix
Store `nextId` in root PROJECT node:
```cpp
void setProjectState(ValueTree state) {
    projectState_ = state;
    nextId_ = state.getProperty("nextId", 1);
}

void incrementId() {
    projectState_.setProperty("nextId", ++nextId_, nullptr);
}
```

### Files to Modify
1. `Source/engine/ProjectState.h` - Remove `rebuildIdCounter()`
2. `Source/engine/ProjectState.cpp` - Use stored ID, fallback to scan for legacy

---

## Roast #9: Hardcoded Input Routing
**Severity**: MEDIUM  
**Location**: `Engine.cpp` `processAudioRecording()`

### Problem
Recording logic assumes sequential channels 0,1. No support for arbitrary input channel selection.

### Fix
Add proper input routing:
```cpp
struct InputRouting {
    juce::String deviceName;
    int channelIndex;
};
// Track stores InputRouting, engine maps device->channel in recording
```

### Files to Modify
1. `Source/engine/Track.h` - Add `InputRouting` struct
2. `Source/engine/Engine.cpp` - Update recording to use routing

---

## Roast #10: Heavy Math in Parameter Setters
**Severity**: MEDIUM  
**Location**: `MixerChannel.cpp` filter coefficient calculation

### Problem
`setHighPassFrequency()` calls `makeHighPass()` (trig functions) immediately, causing CPU spikes on automation.

### Fix
Use dirty flag pattern:
```cpp
void setHighPassFrequency(float freq) {
    targetFrequency = freq;
    filterDirty = true; // Update in audio callback
}

void getNextAudioBlock(...) {
    if (filterDirty.exchange(false)) {
        coefficients = IIRCoefficients::makeHighPass(...);
    }
}
```

### Files to Modify
1. `Source/engine/MixerChannel.h` - Add dirty flags
2. `Source/engine/MixerChannel.cpp` - Defer coefficient updates

---

## Implementation Priority

### Phase 1: Critical Thread Safety (Today)
1. Roast #1 - Shared pointer snapshots
2. Roast #2 - Plugin chain thread safety
5. Roast #5 - Add debug assertions

### Phase 2: Performance (Next Session)
3. Roast #3 - Incremental UI updates
4. Roast #6 - Bulk MIDI operations

### Phase 3: Quality Improvements (Backlog)
7. Roast #4 - Async recording prep
8. Roast #7 - Proper compressor
9. Roast #9 - Input routing
10. Roast #8 - Stored ID counter
11. Roast #10 - Lazy parameter updates

---

## Testing Plan
1. **Thread Safety**: Run with Thread Sanitizer
2. **Performance**: Benchmark with 100-track project
3. **Regression**: Verify playback, recording, plugin hosting still works

---

## Completion Checklist
- [ ] Roast #1: Shared pointer snapshots
- [ ] Roast #2: Plugin snapshot
- [ ] Roast #3: Incremental mixer UI
- [ ] Roast #4: Async recording prep
- [ ] Roast #5: Debug assertions
- [ ] Roast #6: Bulk MIDI insert
- [ ] Roast #7: Proper compressor
- [ ] Roast #8: Stored ID counter
- [ ] Roast #9: Input routing
- [ ] Roast #10: Lazy parameter updates
