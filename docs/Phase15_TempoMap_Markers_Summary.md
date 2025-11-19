# Phase 15: Tempo Map & Global Markers MVP

**Status:** ✅ Complete
**Date:** November 2024

---

## Overview

Phase 15 adds **variable tempo support** and **global timeline markers** to Zenith DAW. Sessions are no longer limited to a single static tempo, and users can navigate using named markers along the timeline.

### Key Features

1. **Tempo Map:**
   - Multiple tempo changes over time (BPM vs beats)
   - Visual tempo lane showing tempo points
   - Add, move, and delete tempo points
   - RT-safe beat↔time/sample conversions in the audio engine

2. **Global Markers:**
   - Named navigation points along the timeline
   - Add, move, rename, and delete markers
   - Visual marker lane with labeled flags

3. **Full Undo/Redo:**
   - All tempo map and marker operations are undoable

4. **CommandAPI Integration:**
   - Programmatic access to tempo map and markers via JSON commands
   - Enables Wingman AI integration

---

## Architecture

### Data Model (ProjectState)

**Tempo Map ValueTree Structure:**
```
PROJECT
├── TEMPO_MAP
│   ├── TEMPO_POINT
│   │   ├── id: "tempopoint_0"
│   │   ├── timeBeats: 0.0
│   │   └── bpm: 120.0
│   ├── TEMPO_POINT
│   │   ├── id: "tempopoint_1"
│   │   ├── timeBeats: 16.0
│   │   └── bpm: 140.0
│   └── ...
```

**Markers ValueTree Structure:**
```
PROJECT
├── MARKERS
│   ├── MARKER
│   │   ├── id: "marker_0"
│   │   ├── timeBeats: 0.0
│   │   └── name: "Intro"
│   ├── MARKER
│   │   ├── id: "marker_1"
│   │   ├── timeBeats: 16.0
│   │   └── name: "Verse"
│   └── ...
```

### ProjectState API

**Tempo Map Methods:**
```cpp
// Add tempo point (undoable)
juce::String addTempoPoint(double timeBeats, double bpm, const juce::String& actionName);

// Move tempo point (undoable)
bool moveTempoPoint(const juce::String& pointId, double newTimeBeats, double newBpm,
                     const juce::String& actionName);

// Delete tempo point (undoable, cannot delete beat 0 point)
bool deleteTempoPoint(const juce::String& pointId, const juce::String& actionName);

// Get all tempo points (sorted by timeBeats)
juce::Array<juce::var> getTempoPoints() const;
```

**Marker Methods:**
```cpp
// Add marker (undoable)
juce::String addMarker(double timeBeats, const juce::String& name, const juce::String& actionName);

// Move marker (undoable)
bool moveMarker(const juce::String& markerId, double newTimeBeats, const juce::String& actionName);

// Rename marker (undoable)
bool renameMarker(const juce::String& markerId, const juce::String& newName,
                   const juce::String& actionName);

// Delete marker (undoable)
bool deleteMarker(const juce::String& markerId, const juce::String& actionName);

// Get all markers (sorted by timeBeats)
juce::Array<juce::var> getMarkers() const;
```

---

## Engine Integration

### TempoMap Class

Located in: `zenith-core/include/TempoMap.h`

Provides **RT-safe** beat↔time/sample conversions:

```cpp
class TempoMap
{
public:
    // RT-safe conversion methods (can be called from audio thread)
    double beatsToSeconds(double beats, double sampleRate) const;
    double secondsToBeats(double seconds, double sampleRate) const;
    int64_t beatsToSamples(double beats, double sampleRate) const;
    double samplesToBeats(int64_t samples, double sampleRate) const;
    double getTempoAt(double beats) const;

    // Message thread only
    void updateFromValueTree(const juce::ValueTree& tempoMapTree);
    void setSingleTempo(double bpm);
};
```

**RT-Safety Mechanism:**

- Uses **atomic shared_ptr** pattern for lock-free access
- Message thread builds `TempoMapSnapshot` with pre-calculated cumulative times
- Audio thread reads snapshot without locks or allocations
- Tempo points are stored sorted by `timeBeats` with cached cumulative seconds

### TempoMapSynchronizer

Located in: `zenith-core/include/TempoMapSynchronizer.h`

Bridges ProjectState (message thread) to Engine's TempoMap:

- Listens to ProjectState ValueTree changes
- Rebuilds TempoMap snapshot when tempo points change
- Runs entirely on message thread
- Started when ProjectState is connected to Engine

---

## UI Components

### TempoLaneComponent

Located in: `zenith-core/include/TempoLaneComponent.h`

**Visual Appearance:**
- Dark gray background with "TEMPO" label
- Tempo points shown as colored nodes
- Horizontal lines connecting points (step-wise tempo changes)
- BPM values displayed above points

**User Interactions:**
- **Double-click:** Add tempo point at cursor position (defaults to 120 BPM)
- **Click:** Select tempo point (turns yellow)
- **Drag:** Move tempo point horizontally (time) and vertically (BPM)
- **Delete/Backspace:** Delete selected tempo point (except beat 0)

**Constraints:**
- BPM range: 40-240
- Cannot delete or move the tempo point at beat 0 (can only change its BPM)
- View range: 0-64 beats (MVP fixed range)

### MarkerLaneComponent

Located in: `zenith-core/include/MarkerLaneComponent.h`

**Visual Appearance:**
- Medium gray background with "MARKERS" label
- Markers shown as green flag icons with text labels
- Labels display marker names

**User Interactions:**
- **Double-click (empty area):** Add marker with auto-generated name ("Marker 1", "Marker 2", etc.)
- **Double-click (on marker):** Rename marker (shows dialog)
- **Click:** Select marker (turns yellow)
- **Drag:** Move marker horizontally
- **Delete/Backspace:** Delete selected marker

**Constraints:**
- View range: 0-64 beats (MVP fixed range)
- Marker names are user-editable

---

## CommandAPI Integration

### Tempo Map Commands

**Add Tempo Point:**
```json
{
  "command": "add_tempo_point",
  "params": { "timeBeats": 8.0, "bpm": 140.0 }
}
```
Returns: `{ "pointId": "tempopoint_123" }`

**Delete Tempo Point:**
```json
{
  "command": "delete_tempo_point",
  "params": { "pointId": "tempopoint_123" }
}
```
Returns: `{ "success": true }`

**Get Tempo Map:**
```json
{
  "command": "get_tempo_map",
  "params": {}
}
```
Returns: `{ "points": [ { "id": "...", "timeBeats": 0.0, "bpm": 120.0 }, ... ] }`

### Marker Commands

**Add Marker:**
```json
{
  "command": "add_marker",
  "params": { "timeBeats": 16.0, "name": "Verse" }
}
```
Returns: `{ "markerId": "marker_123" }`

**Move Marker:**
```json
{
  "command": "move_marker",
  "params": { "markerId": "marker_123", "timeBeats": 20.0 }
}
```
Returns: `{ "success": true }`

**Rename Marker:**
```json
{
  "command": "rename_marker",
  "params": { "markerId": "marker_123", "name": "Chorus" }
}
```
Returns: `{ "success": true }`

**Delete Marker:**
```json
{
  "command": "delete_marker",
  "params": { "markerId": "marker_123" }
}
```
Returns: `{ "success": true }`

**Get Markers:**
```json
{
  "command": "get_markers",
  "params": {}
}
```
Returns: `{ "markers": [ { "id": "...", "timeBeats": 0.0, "name": "..." }, ... ] }`

---

## Manual Test Plan

### Test 1: Create Default Tempo Map
**Steps:**
1. Launch Zenith DAW
2. Create new project
3. Observe tempo lane in UI

**Expected:**
- Tempo lane visible at top of arranger
- Single tempo point at beat 0 showing 120 BPM

---

### Test 2: Add Tempo Point
**Steps:**
1. Double-click in tempo lane at beat 8
2. Observe new tempo point appears

**Expected:**
- New tempo point created at beat 8
- Shows 120 BPM (default)
- Point is rendered as colored node

---

### Test 3: Move Tempo Point (Time)
**Steps:**
1. Add tempo point at beat 8
2. Drag point horizontally to beat 12
3. Release mouse

**Expected:**
- Point moves to beat 12
- Tempo map updates
- Undo/redo works (Ctrl+Z / Ctrl+Shift+Z)

---

### Test 4: Adjust Tempo Point (BPM)
**Steps:**
1. Add tempo point at beat 8
2. Drag point vertically upward
3. Observe BPM value change

**Expected:**
- BPM value increases (up to 240 max)
- Label updates to show new BPM
- Tempo map recalculates beat↔time

---

### Test 5: Delete Tempo Point
**Steps:**
1. Add tempo point at beat 8
2. Click to select point (turns yellow)
3. Press Delete or Backspace

**Expected:**
- Point is removed
- Tempo map updates
- Undo/redo works

---

### Test 6: Cannot Delete Beat 0 Tempo Point
**Steps:**
1. Click on tempo point at beat 0
2. Press Delete or Backspace

**Expected:**
- Point is NOT deleted
- Warning logged to console
- Beat 0 point remains (required)

---

### Test 7: Add Marker
**Steps:**
1. Double-click in marker lane at beat 16
2. Observe new marker appears

**Expected:**
- New marker created with name "Marker 1"
- Rendered as green flag with label
- Sorted by time in lane

---

### Test 8: Move Marker
**Steps:**
1. Add marker at beat 16
2. Drag marker horizontally to beat 20
3. Release mouse

**Expected:**
- Marker moves to beat 20
- Label follows marker
- Undo/redo works

---

### Test 9: Rename Marker
**Steps:**
1. Add marker at beat 16 ("Marker 1")
2. Double-click on marker label
3. Dialog appears (or inline edit)
4. Enter "Verse"
5. Confirm

**Expected:**
- Marker name changes to "Verse"
- Label updates in UI
- Undo/redo works

---

### Test 10: Delete Marker
**Steps:**
1. Add marker at beat 16
2. Click to select marker (turns yellow)
3. Press Delete or Backspace

**Expected:**
- Marker is removed
- Undo/redo works

---

### Test 11: CommandAPI - Add Tempo Point
**Steps:**
1. Use CommandAPI to execute:
   ```json
   {
     "command": "add_tempo_point",
     "params": { "timeBeats": 8.0, "bpm": 140.0 }
   }
   ```

**Expected:**
- Response: `{ "status": "ok", "data": { "pointId": "tempopoint_..." } }`
- Tempo point appears in UI
- Undo/redo works

---

### Test 12: CommandAPI - Get Tempo Map
**Steps:**
1. Add 2-3 tempo points via UI
2. Use CommandAPI to execute:
   ```json
   {
     "command": "get_tempo_map",
     "params": {}
   }
   ```

**Expected:**
- Response contains all tempo points
- Points sorted by timeBeats
- Data matches UI display

---

### Test 13: Undo/Redo Multiple Operations
**Steps:**
1. Add tempo point at beat 8
2. Add marker at beat 16
3. Move tempo point to beat 10
4. Rename marker to "Chorus"
5. Press Ctrl+Z four times
6. Press Ctrl+Shift+Z four times

**Expected:**
- All operations undo in reverse order
- All operations redo in forward order
- UI updates correctly each step

---

### Test 14: Save/Load Project with Tempo Map
**Steps:**
1. Add 2-3 tempo points
2. Add 2-3 markers
3. Save project to .zth file
4. Close project
5. Load .zth file

**Expected:**
- Tempo points and markers restored
- Tempo map calculations correct
- UI displays all data

---

## Files Created/Modified

### Created Files:
- `zenith-core/include/TempoMap.h` - RT-safe tempo map data structure
- `zenith-core/src/TempoMap.cpp` - Tempo map implementation
- `zenith-core/include/TempoMapSynchronizer.h` - ProjectState→Engine sync
- `zenith-core/src/TempoMapSynchronizer.cpp` - Synchronizer implementation
- `zenith-core/include/TempoLaneComponent.h` - Tempo lane UI
- `zenith-core/src/TempoLaneComponent.cpp` - Tempo lane implementation
- `zenith-core/include/MarkerLaneComponent.h` - Marker lane UI
- `zenith-core/src/MarkerLaneComponent.cpp` - Marker lane implementation
- `docs/Phase15_TempoMap_Markers_Summary.md` - This documentation

### Modified Files:
- `zenith-core/include/ProjectState.h` - Added tempo map & marker identifiers and API
- `zenith-core/src/ProjectState.cpp` - Implemented tempo map & marker methods
- `zenith-core/include/Engine.h` - Added TempoMap member and getters
- `zenith-core/src/Engine.cpp` - Integrated TempoMap and synchronizer
- `zenith-core/include/MainWindow.h` - Added tempo/marker lane components
- `zenith-core/src/MainWindow.cpp` - Integrated lanes into UI layout
- `zenith-core/include/CommandAPI.h` - Added tempo/marker command declarations
- `zenith-core/src/CommandAPI.cpp` - Implemented tempo/marker commands
- `zenith-core/CMakeLists.txt` - Added new source files

---

## Known Limitations (MVP)

These features are **intentionally deferred** to future phases:

1. **No time signature changes** - Project remains 4/4
2. **No spline/bezier tempo curves** - Only step or linear segments
3. **No clip time-stretching** - Audio clips don't stretch with tempo changes
4. **No per-track tempo** - Tempo map is global only
5. **No marker regions** - Only point markers supported
6. **No marker-based export** - Export doesn't use markers yet
7. **Fixed view range** - No zoom/scroll in tempo/marker lanes (0-64 beats)
8. **Simple rename dialog** - Marker rename uses basic AlertWindow
9. **No deep MIDI scheduling** - MIDI scheduling may not fully respect tempo map yet

---

## RT-Safety Compliance

**✅ All RT-safety requirements met:**

- ❌ **Audio thread does NOT:**
  - Lock mutexes
  - Allocate memory
  - Log to console
  - Perform file I/O
  - Access ValueTree directly

- ✅ **Audio thread ONLY:**
  - Reads atomic shared_ptr
  - Uses pre-built tempo map snapshots
  - Performs RT-safe beat↔time calculations

- ✅ **Message thread handles:**
  - All ValueTree modifications
  - All UI interactions
  - Tempo map snapshot building
  - Undo/redo operations

---

## Performance Notes

- **Tempo map lookups:** O(log n) binary search (if needed) or O(n) linear scan for small maps
- **Snapshot updates:** Only occur when tempo points change (not every frame)
- **UI repaints:** Triggered only on ValueTree changes
- **Memory overhead:** Minimal - tempo map snapshots are small (<1 KB typical)

---

## Future Work

**Potential Phase 16+ enhancements:**

1. Add time signature support
2. Implement tempo curves (bezier/spline)
3. Add clip time-stretching
4. Support marker regions (start/end)
5. Add marker-based export/bounce
6. Implement zoom/scroll in timeline lanes
7. Add marker colors
8. Support MIDI tempo changes (external sync)
9. Add tempo tap/detection
10. Implement marker-based looping

---

## Conclusion

Phase 15 successfully adds **variable tempo** and **global markers** to Zenith DAW, providing a solid foundation for more advanced timeline features in future phases. The implementation maintains strict **RT-safety**, full **undo/redo** support, and clean **separation of concerns** between UI, model, and engine layers.

All acceptance criteria have been met, and the feature is ready for integration testing and user feedback.

---

**Phase 15 Status:** ✅ **COMPLETE**
