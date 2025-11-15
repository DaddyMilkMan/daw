# Phase 15 – Tempo Map + Markers MVP

## Overview

Phase 15 implements a minimal but solid implementation of tempo map and markers functionality for Zenith DAW. This phase adds:

- **Tempo Map**: Support for multiple tempo changes throughout a song, stored in beats
- **Markers**: Named markers on the timeline for navigation
- **ProjectState Integration**: Full ValueTree-based storage with undo/redo support
- **Engine Integration**: RT-safe tempo queries for audio thread
- **CommandAPI**: JSON commands for external control and Wingman AI integration

**What's NOT included** (explicitly out of scope for this phase):
- Audio time-stretching or warping based on tempo changes
- Advanced tempo curves or ramping
- Comping, pre-roll, or punch in/out
- Time signature-based grid display changes in main UI
- UI components for visual tempo and marker editing (backend is complete)

## Data Model

### Tempo Map Structure

The tempo map is stored in the ProjectState ValueTree under a `TEMPO_MAP` node:

```
PROJECT
└── TEMPO_MAP
    ├── TEMPO_CHANGE
    │   ├── id: "tempo_0"
    │   ├── beatPosition: 0.0
    │   ├── bpm: 120.0
    │   ├── timeSigNumerator: 4
    │   └── timeSigDenominator: 4
    ├── TEMPO_CHANGE
    │   ├── id: "tempo_1"
    │   ├── beatPosition: 16.0
    │   ├── bpm: 140.0
    │   ├── timeSigNumerator: 4
    │   └── timeSigDenominator: 4
    └── ...
```

**Properties:**
- `id` (string): Unique identifier for the tempo change
- `beatPosition` (double): Position in beats from song start (>= 0.0)
- `bpm` (double): Beats per minute (> 0.0, typically 40–300)
- `timeSigNumerator` (int): Time signature numerator (e.g., 4 for 4/4)
- `timeSigDenominator` (int): Time signature denominator (e.g., 4 for 4/4)

**Constraints:**
- Tempo changes are always sorted by `beatPosition`
- There must always be at least one tempo change at `beatPosition = 0.0`
- Deleting the last tempo at beat 0 creates a default replacement

### Markers Structure

Markers are stored under a `MARKERS` node:

```
PROJECT
└── MARKERS
    ├── MARKER
    │   ├── id: "marker_0"
    │   ├── name: "Intro"
    │   ├── beatPosition: 0.0
    │   └── color: "#FFCC00"
    ├── MARKER
    │   ├── id: "marker_1"
    │   ├── name: "Verse 1"
    │   ├── beatPosition: 8.0
    │   └── color: "#FF6600"
    └── ...
```

**Properties:**
- `id` (string): Unique identifier for the marker
- `name` (string): Display name (e.g., "Verse", "Chorus 1")
- `beatPosition` (double): Position in beats from song start (>= 0.0)
- `color` (string): Hex color code (e.g., "#FFCC00")

**Constraints:**
- Markers are sorted by `beatPosition`
- Multiple markers can exist at the same beat position
- Markers are independent and can be added/deleted freely

## ProjectState API

### Tempo Map Methods

All methods are **message thread only** and **undoable** via UndoManager.

#### `juce::ValueTree getTempoMapNode()`
Returns the TEMPO_MAP ValueTree, creating it with a default tempo at beat 0 if it doesn't exist.

#### `juce::Array<TempoChangeSpec> getTempoChanges() const`
Returns all tempo changes sorted by beat position.

#### `juce::String addTempoChange(double beatPosition, double bpm, int numerator, int denominator, const juce::String& actionName)`
Adds a tempo change and returns its ID. Automatically inserts in sorted order.

#### `bool moveTempoChange(const juce::String& tempoId, double newBeatPosition, const juce::String& actionName)`
Moves a tempo change to a new beat position. Prevents moving the only tempo away from beat 0.

#### `bool setTempoChangeBpm(const juce::String& tempoId, double newBpm, const juce::String& actionName)`
Updates the BPM of an existing tempo change.

#### `bool setTempoChangeTimeSig(const juce::String& tempoId, int numerator, int denominator, const juce::String& actionName)`
Updates the time signature of a tempo change.

#### `bool deleteTempoChange(const juce::String& tempoId, const juce::String& actionName)`
Deletes a tempo change. Ensures there's always a tempo at beat 0 by creating a default if needed.

#### `double getTempoAtBeat(double beat) const`
Returns the BPM value at a specific beat position.

#### `double beatToSeconds(double beat) const`
Converts a beat position to time in seconds, integrating through tempo changes.

#### `double secondsToBeat(double seconds) const`
Converts time in seconds to beat position, integrating through tempo changes.

### Markers Methods

All methods are **message thread only** and **undoable** via UndoManager.

#### `juce::Array<MarkerSpec> getMarkers() const`
Returns all markers sorted by beat position.

#### `juce::String addMarker(double beatPosition, const juce::String& name, const juce::String& colorHex, const juce::String& actionName)`
Adds a marker and returns its ID.

#### `bool moveMarker(const juce::String& markerId, double newBeatPosition, const juce::String& actionName)`
Moves a marker to a new beat position.

#### `bool renameMarker(const juce::String& markerId, const juce::String& newName, const juce::String& actionName)`
Renames a marker.

#### `bool recolorMarker(const juce::String& markerId, const juce::String& newColorHex, const juce::String& actionName)`
Changes a marker's color.

#### `bool deleteMarker(const juce::String& markerId, const juce::String& actionName)`
Deletes a marker.

## Engine Integration

### TempoMapRuntime

The Engine maintains a precomputed tempo map structure for RT-safe access:

```cpp
struct TempoSegment
{
    double startBeat;           // Start beat of this segment
    double bpm;                 // BPM for this segment
    double secondsAtStartBeat;  // Accumulated seconds at start beat
    int timeSigNumerator;
    int timeSigDenominator;
};
```

**Thread Safety:**
- Tempo segments are updated on the **message thread** only
- Audio thread reads segments via mutex-protected access (brief lock, precomputed data)
- No allocations or heavy computation on audio thread

### Engine API

#### `void setTempoMap(const juce::Array<ProjectState::TempoChangeSpec>& tempoChanges)` [MESSAGE THREAD]
Precomputes tempo segments from ProjectState tempo changes. Called by TempoMapSynchronizer when tempo map changes.

#### `double getTempoAtSample(juce::int64 samplePos) const noexcept` [RT-SAFE]
Returns the BPM at a specific sample position. Can be called from audio thread.

#### `double sampleToBeat(juce::int64 samplePos) const noexcept` [RT-SAFE]
Converts sample position to beat position. Can be called from audio thread.

#### `juce::int64 beatToSample(double beat) const noexcept` [RT-SAFE]
Converts beat position to sample position. Can be called from audio thread.

#### `void setPlayheadPosition(juce::int64 samplePos)` [MESSAGE THREAD]
Sets the playhead position in samples.

#### `juce::int64 getPlayheadPosition() const noexcept` [RT-SAFE]
Returns the current playhead position in samples.

### TempoMapSynchronizer

Similar to `TrackAutomationSynchronizer`, the `TempoMapSynchronizer` class:
- Listens to ProjectState ValueTree changes for tempo map and markers
- Calls `Engine::setTempoMap()` when tempo changes occur
- Runs entirely on the message thread
- Automatically initialized when ProjectState is set on Engine

**RT-Safety Guarantee:**
The audio thread never accesses ValueTree or ProjectState directly. All tempo queries use precomputed segments that are atomically swapped on the message thread.

## UI Behavior

**Note:** This phase implements the complete backend for tempo map and markers. Visual UI components (tempo lane, markers ribbon) are intentionally minimal/not implemented in this phase to focus on backend completeness. All functionality is accessible via CommandAPI.

Future UI implementation would include:
- Tempo lane showing tempo graph over timeline
- Interactive tempo point editing (drag to move/adjust BPM)
- Markers ribbon with draggable marker flags
- Marker name display and editing
- Keyboard navigation to markers

## CommandAPI

Phase 15 adds the following JSON commands:

### Tempo Map Commands

#### `add_tempo_change`
**Request:**
```json
{
  "command": "add_tempo_change",
  "params": {
    "beatPosition": 8.0,
    "bpm": 140.0,
    "timeSigNumerator": 4,
    "timeSigDenominator": 4
  }
}
```
**Response:**
```json
{
  "status": "ok",
  "data": {
    "tempoId": "tempo_123"
  }
}
```

#### `get_tempo_map`
**Request:**
```json
{
  "command": "get_tempo_map",
  "params": {}
}
```
**Response:**
```json
{
  "status": "ok",
  "data": {
    "tempoChanges": [
      {
        "id": "tempo_0",
        "beatPosition": 0.0,
        "bpm": 120.0,
        "timeSigNumerator": 4,
        "timeSigDenominator": 4
      },
      {
        "id": "tempo_1",
        "beatPosition": 16.0,
        "bpm": 140.0,
        "timeSigNumerator": 4,
        "timeSigDenominator": 4
      }
    ]
  }
}
```

### Markers Commands

#### `add_marker`
**Request:**
```json
{
  "command": "add_marker",
  "params": {
    "beatPosition": 4.0,
    "name": "Verse",
    "color": "#FFCC00"
  }
}
```
**Response:**
```json
{
  "status": "ok",
  "data": {
    "markerId": "marker_123"
  }
}
```

#### `get_markers`
**Request:**
```json
{
  "command": "get_markers",
  "params": {}
}
```
**Response:**
```json
{
  "status": "ok",
  "data": {
    "markers": [
      {
        "id": "marker_0",
        "name": "Intro",
        "beatPosition": 0.0,
        "color": "#FFCC00"
      },
      {
        "id": "marker_1",
        "name": "Verse 1",
        "beatPosition": 8.0,
        "color": "#FF6600"
      }
    ]
  }
}
```

#### `delete_marker`
**Request:**
```json
{
  "command": "delete_marker",
  "params": {
    "markerId": "marker_123"
  }
}
```
**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true
  }
}
```

#### `goto_marker`
**Request:**
```json
{
  "command": "goto_marker",
  "params": {
    "markerId": "marker_123"
  }
}
```
**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true,
    "beatPosition": 4.0
  }
}
```

## Manual Test Plan

### Setup
1. Build and run Zenith DAW
2. Ensure audio engine initializes successfully
3. Create a new project or use existing project

### Tempo Map Tests

**Test 1: Initial tempo map creation**
- Expected: ProjectState automatically creates a default tempo at beat 0 with project tempo (120 BPM)
- Verification: Use CommandAPI `get_tempo_map` to verify default tempo exists

**Test 2: Add tempo change**
- Action: Add tempo change at beat 16.0 with 140 BPM via `add_tempo_change`
- Expected: New tempo change added and sorted correctly
- Verification: `get_tempo_map` shows two tempo changes in order

**Test 3: Add multiple tempo changes**
- Action: Add tempo changes at beats 8.0 (130 BPM), 24.0 (120 BPM), and 32.0 (150 BPM)
- Expected: All tempo changes sorted by beat position
- Verification: `get_tempo_map` returns 5 tempo changes in ascending beat order

**Test 4: Beat ↔ Seconds conversion**
- Action: Query `beatToSeconds(16.0)` and `secondsToBeat(8.0)` via ProjectState API
- Expected: Conversions integrate correctly through tempo changes
- Verification: Convert beat 16 → seconds → back to beat, should equal 16.0

**Test 5: Delete non-zero tempo change**
- Action: Delete tempo change at beat 16.0
- Expected: Tempo change removed, beat 0 tempo persists
- Verification: `get_tempo_map` shows 4 remaining tempo changes, beat 0 still present

**Test 6: Attempt to delete last tempo at beat 0**
- Action: Delete all tempo changes except the one at beat 0, then delete beat 0
- Expected: System creates a default tempo at beat 0 with previous BPM values
- Verification: `get_tempo_map` shows one tempo at beat 0

**Test 7: Undo/redo tempo operations**
- Action: Perform Ctrl+Z (undo) after adding tempo changes
- Expected: Tempo changes are undone in reverse order
- Action: Perform Ctrl+Shift+Z (redo)
- Expected: Tempo changes are restored
- Verification: Check state after each undo/redo

### Markers Tests

**Test 8: Add markers**
- Action: Add markers at beats 0, 4, 8, 16 with names "Intro", "Verse", "Chorus", "Bridge"
- Expected: Markers created and sorted by beat position
- Verification: `get_markers` returns 4 markers in order

**Test 9: Rename marker**
- Action: Rename "Verse" marker to "Verse 1"
- Expected: Marker name updated
- Verification: `get_markers` shows updated name

**Test 10: Recolor marker**
- Action: Change "Chorus" marker color to "#FF0000"
- Expected: Marker color updated
- Verification: `get_markers` shows new color

**Test 11: Move marker**
- Action: Move "Bridge" marker from beat 16 to beat 20
- Expected: Marker moved, list re-sorted
- Verification: `get_markers` shows marker at new position

**Test 12: Delete marker**
- Action: Delete "Intro" marker
- Expected: Marker removed
- Verification: `get_markers` returns 3 markers

**Test 13: Go to marker**
- Action: Use `goto_marker` to jump to "Verse 1" marker
- Expected: Playhead position set to marker's beat position
- Verification: Check `getPlayheadPosition()` converts to correct beat

**Test 14: Undo/redo marker operations**
- Action: Undo last few marker operations
- Expected: Markers restored to previous state
- Action: Redo operations
- Expected: Changes reapplied
- Verification: Marker list matches expected state

### Integration Tests

**Test 15: Save and load project**
- Action: Save project to .zth file
- Action: Close and reopen project
- Expected: Tempo map and markers fully restored
- Verification: Compare `get_tempo_map` and `get_markers` before and after

**Test 16: Engine tempo map sync**
- Action: Add tempo change via CommandAPI
- Expected: TempoMapSynchronizer updates Engine automatically
- Verification: Engine's `sampleToBeat()` reflects new tempo map

**Test 17: Playback during tempo changes**
- Action: Start playback with multiple tempo changes in project
- Expected: Audio playback continues smoothly (no crashes or audio glitches)
- Verification: Visual confirmation of playback, no console errors

**Test 18: RT-safety verification**
- Action: Add/delete tempo changes while audio is playing
- Expected: No audio dropouts or thread contention
- Verification: CPU usage remains stable, no xruns or glitches

## RT-Safety Confirmation

Phase 15 maintains strict RT-safety:

✅ **Audio thread NEVER:**
- Allocates memory (no `new`, `malloc`, or dynamic containers like `std::vector::push_back`)
- Locks mutexes for extended periods (only brief locks to read precomputed segments)
- Performs file I/O or logging
- Accesses ValueTree or ProjectState directly

✅ **Audio thread CAN:**
- Read precomputed tempo segments via brief mutex locks
- Perform `sampleToBeat` and `beatToSample` calculations using precomputed data
- Read atomic playback position

✅ **Message thread handles:**
- All ValueTree modifications
- All tempo map precomputation in `Engine::setTempoMap()`
- All TempoMapSynchronizer updates
- All CommandAPI calls

## Limitations / Next Phases

**Explicitly out of scope for Phase 15:**
1. **Audio time-stretching**: Tempo changes do NOT warp existing audio clips to match new tempo
2. **Tempo curves/ramping**: Only discrete tempo changes, no gradual tempo ramps
3. **Visual UI components**: Backend is complete, but tempo lane and markers UI are minimal/not implemented
4. **Time signature grid rendering**: Time signature changes are stored but don't affect main grid display yet
5. **Comping workflows**: No region comping or playlist lanes
6. **Pre-roll/punch in-out**: Not implemented in this phase

**Future enhancements (later phases):**
- Visual tempo graph editor in timeline
- Drag-and-drop tempo and marker editing
- Tempo-based audio time-stretching (elastic audio)
- Gradual tempo curves (accelerando/ritardando)
- Click/metronome track with tempo awareness
- Export with tempo map metadata

## Files Modified/Created

### Modified Files
- `zenith-core/include/ProjectState.h`
- `zenith-core/src/ProjectState.cpp`
- `zenith-core/include/Engine.h`
- `zenith-core/src/Engine.cpp`
- `zenith-core/include/CommandAPI.h`
- `zenith-core/src/CommandAPI.cpp`
- `zenith-core/CMakeLists.txt`

### New Files
- `zenith-core/include/TempoMapSynchronizer.h`
- `zenith-core/src/TempoMapSynchronizer.cpp`
- `zenith-core/docs/Phase15_TempoMap_Markers_Summary.md` (this file)

## Summary

Phase 15 successfully implements a complete backend for tempo map and markers in Zenith DAW:

- ✅ Multiple tempo changes stored beat-accurately
- ✅ Full undo/redo support via ValueTree + UndoManager
- ✅ RT-safe tempo queries in audio thread
- ✅ Named markers for timeline navigation
- ✅ CommandAPI integration for external control
- ✅ Automatic synchronization between ProjectState and Engine
- ✅ Save/load persistence (via existing ProjectState XML serialization)
- ✅ Complete manual test plan covering all functionality

The implementation is production-ready for backend use, with visual UI components deferred to future phases.
