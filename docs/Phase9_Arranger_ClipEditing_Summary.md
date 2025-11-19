# Phase 9 – Arranger Clip Editing & Timeline UX

**Status:** ✅ Complete
**Date:** 2025-11-14
**Scope:** Arranger MVP with interactive clip editing

---

## Overview

Phase 9 introduces a production-ready **Timeline/Arranger** component with full clip editing capabilities. This brings Zenith's UI up to par with modern DAWs by enabling users to visually create, edit, and arrange clips directly on the timeline without requiring JSON commands or external tools.

**Key Deliverables:**
- `ArrangerComponent` - Timeline UI with track lanes and clip visualization
- Clip operations: create, select, move, resize, delete, duplicate
- Zoom and scroll (horizontal + vertical)
- All operations integrated with `ProjectState` and `UndoManager`
- Message-thread only (no RT audio thread changes)

---

## Architecture

### Components

#### 1. **ProjectState Clip Management APIs**

New APIs added to `ProjectState` for clip lifecycle management:

```cpp
// Create clip
juce::String createEmptyClip(const juce::String& trackId,
                              double startBeats,
                              double lengthBeats,
                              bool isMidi,
                              const juce::String& name,
                              const juce::String& actionName);

// Move clip (time + track)
void moveClip(const juce::String& clipId,
              const juce::String& newTrackId,
              double newStartBeats,
              const juce::String& actionName);

// Resize clip
void setClipRange(const juce::String& clipId,
                  double newStartBeats,
                  double newLengthBeats,
                  const juce::String& actionName);

// Delete clip
void deleteClip(const juce::String& clipId,
                const juce::String& actionName);

// Find clip across all tracks
std::pair<juce::ValueTree, juce::ValueTree> findClip(const juce::String& clipId);

// Get track by ID
juce::ValueTree getTrack(const juce::String& trackId);
```

**Characteristics:**
- All APIs operate on the `ValueTree` model
- Use `UndoManager` for undo/redo support
- Message-thread only (enforced with assertions)
- Return meaningful IDs for tracking operations

#### 2. **ClipView Struct**

Lightweight UI representation of a clip:

```cpp
struct ClipView
{
    juce::String clipId;
    juce::String trackId;
    double startBeats;
    double lengthBeats;
    bool isMidi;
    bool isSelected;
    juce::Rectangle<float> bounds; // Screen coordinates

    // Helper methods for resize zones
    bool isInLeftResizeZone(juce::Point<float> point, float zoneWidth = 6.0f);
    bool isInRightResizeZone(juce::Point<float> point, float zoneWidth = 6.0f);
};
```

**Design:**
- Maps `ProjectState` clip data → screen coordinates
- Cached for efficient hit-testing and drawing
- Rebuilt from `ValueTree` whenever state changes
- Source of truth is always `ProjectState`, not `ClipView`

#### 3. **ArrangerComponent**

Main timeline UI component (`zenith-core/include/ArrangerComponent.h`, `src/ArrangerComponent.cpp`):

**Responsibilities:**
- Listen to `ProjectState` ValueTree changes
- Maintain array of `ClipView` for drawing/interaction
- Handle mouse/keyboard input
- Manage selection state
- Trigger clip operations via `ProjectState` APIs
- Paint timeline grid, tracks, clips, and UI feedback

**Key Features:**
- **Selection:** Single-click, Ctrl/Cmd-click (toggle), Shift-drag (marquee)
- **Move:** Drag clip body to move in time or between tracks
- **Resize:** Drag left/right edges to adjust clip start/length
- **Create:** Double-click empty area to create MIDI clip
- **Delete:** Delete/Backspace key
- **Duplicate:** Ctrl/Cmd+D
- **Zoom:** Ctrl/Cmd+mousewheel (horizontal), +/- keys
- **Scroll:** Mousewheel (vertical), Shift+mousewheel (horizontal)
- **Undo/Redo:** Ctrl/Cmd+Z, Ctrl/Cmd+Shift+Z

**Drag Modes:**
```cpp
enum class DragMode
{
    None,
    MoveClips,
    ResizeClipLeft,
    ResizeClipRight,
    Marquee
};
```

**Coordinate System:**
- Horizontal: beats → pixels via `pixelsPerBeat`
- Vertical: track index → y-coordinate via `trackHeight`
- Time ruler at top (30px)
- Tracks below with dividers

**Colors:**
- Audio clips: Blue (`0xff4a90e2`)
- MIDI clips: Green (`0xff7ed321`)
- Selected clips: White border (`0xffffffff`)
- Track lanes: Dark grey with alternating brightness

---

## Clip Operations

### 1. Create Clip

**Trigger:** Double-click empty area of track lane

**Behavior:**
1. Determine target track from Y position
2. Calculate start position from X position (snapped to grid)
3. Use default length (4 beats = 1 bar in 4/4)
4. Call `projectState.createEmptyClip(trackId, startBeats, 4.0, true, "Clip", "Create clip")`
5. Clip appears immediately via ValueTree listener

**Undo:** Single "Create clip" transaction

### 2. Select Clips

**Single Selection:**
- Click clip → clear previous selection, select this clip
- Ctrl/Cmd+click clip → toggle clip in selection

**Multi-Selection:**
- Shift+drag on empty area → draw marquee rectangle
- On mouse up → select all clips intersecting marquee

**Clear Selection:**
- Click empty area (no Shift)
- Press Escape key

### 3. Move Clips

**Trigger:** Drag clip body (not edges)

**Behavior:**
1. If clip not selected, select it first
2. Cache original positions for all selected clips
3. On drag: compute delta in beats and track index
4. Update `ClipView` positions for live visual feedback (UI-only)
5. On mouse up:
   - Begin transaction "Move clips"
   - Snap final positions to grid
   - Call `projectState.moveClip(...)` for each selected clip
   - Clip changes propagate via ValueTree listener

**Multi-Clip Move:**
- All selected clips move together by same delta

**Snap to Grid:**
- Grid resolution: `gridSnapBeats = 0.25` (1/16 note in 4/4)
- Snaps on commit (mouse up), not during drag

**Constraints:**
- `startBeats >= 0`
- Track index clamped to valid range

**Undo:** Single "Move clips" transaction (restores all clips)

### 4. Resize Clips

**Trigger:** Drag left or right edge of clip (6px resize zone)

**Left Edge Resize:**
- Changes `startBeats` and adjusts `lengthBeats` to keep end position fixed
- Formula: `newLength = originalEnd - newStartBeats`

**Right Edge Resize:**
- Changes `lengthBeats` only (start position fixed)

**Behavior:**
1. Detect edge zone in `mouseDown`
2. Cache original start/length
3. On drag: update `ClipView` for visual feedback
4. On mouse up:
   - Snap start and length to grid
   - Call `projectState.setClipRange(clipId, newStart, newLength, "Resize clip")`

**Constraints:**
- Minimum length: `0.25` beats (enforced in `ProjectState`)

**Limitations:**
- V1 only resizes single clip even if multiple selected

**Undo:** Single "Resize clip" transaction

### 5. Delete Clips

**Trigger:** Delete or Backspace key (while Arranger has focus)

**Behavior:**
1. Begin transaction "Delete clips"
2. For each selected clip: call `projectState.deleteClip(clipId, "Delete clips")`
3. Clear selection
4. Clips removed immediately via ValueTree listener

**Undo:** Single "Delete clips" transaction (restores all)

### 6. Duplicate Clips

**Trigger:** Ctrl/Cmd+D

**Behavior:**
1. Begin transaction "Duplicate clips"
2. For each selected clip:
   - Read trackId, startBeats, lengthBeats, isMidi, name
   - Calculate new start: `originalStart + originalLength`
   - Call `projectState.createEmptyClip(...)` with new position
3. Select the newly created clips (deselect originals)

**Undo:** Single "Duplicate clips" transaction (removes all duplicates)

---

## Zoom & Scroll

### Horizontal Zoom

**Trigger:**
- Ctrl/Cmd+mousewheel
- +/- keys

**Behavior:**
- Adjusts `pixelsPerBeat` (range: 10.0 - 200.0)
- Zooms around mouse position (mousewheel) or view center (keys)
- Recomputes clip bounds and repaints

### Horizontal Scroll

**Trigger:**
- Shift+mousewheel

**Behavior:**
- Adjusts `viewStartBeats` (>= 0)
- Recomputes clip bounds and repaints

### Vertical Scroll

**Trigger:**
- Mousewheel (no modifier)

**Behavior:**
- Adjusts `firstVisibleTrackIndex` (clamped to track count)
- Recomputes clip bounds and repaints

---

## Undo/Redo Integration

All clip operations use `ProjectState`'s `UndoManager`:

**Transaction Grouping:**
- Each user action creates a single named transaction
- Multi-clip operations (move/delete/duplicate) grouped into one transaction
- Undo/redo restores full state atomically

**Keyboard Shortcuts:**
- Undo: Ctrl/Cmd+Z
- Redo: Ctrl/Cmd+Shift+Z or Ctrl/Cmd+Y

**Transaction Names:**
- "Create clip"
- "Move clips"
- "Resize clip"
- "Delete clips"
- "Duplicate clips"

---

## ValueTree Listener Integration

`ArrangerComponent` listens to `ProjectState.getState()` and rebuilds `clipViews` on:

- `valueTreePropertyChanged` - Clip property modified (start, length, etc.)
- `valueTreeChildAdded` - Track or clip added
- `valueTreeChildRemoved` - Track or clip removed
- `valueTreeChildOrderChanged` - Track/clip order changed

**Rebuild Pattern:**
1. Clear `clipViews` array
2. Iterate all tracks and clips in `ValueTree`
3. Create `ClipView` for each clip
4. Update selection flags from `selectedClipIds`
5. Recompute screen bounds
6. Repaint

---

## RT-Safety

**All Phase 9 code is message-thread only:**
- `ArrangerComponent` asserts message thread in all operations
- `ProjectState` clip APIs assert message thread
- No audio thread interaction
- No new locks on audio callback path
- No allocations in hot paths

**Engine Integration:**
- Engine remains untouched in this phase
- Legacy `Track` / `Clip` classes in `Source/engine/` not modified
- Future phases will bridge `ProjectState` ↔ engine

---

## Integration with MainWindow

**Changes to MainWindow:**
1. `MainComponent` now takes `ProjectState&` reference
2. Constructs `ArrangerComponent` and adds it to layout
3. ArrangerComponent occupies central area between top bar and transport
4. Status label updated to "Phase 9: Arranger MVP"
5. Default tracks created on startup (Audio 1, MIDI 1, Audio 2) for testing

**Layout:**
```
┌─────────────────────────────────────┐
│ [Status] [CPU] [Track Count]       │ ← Top bar (40px)
├─────────────────────────────────────┤
│                                     │
│    ArrangerComponent                │
│    (Timeline with tracks/clips)     │
│                                     │
├─────────────────────────────────────┤
│ [Device] [Play] [Stop] [Record]    │ ← Transport (50px)
└─────────────────────────────────────┘
```

---

## Manual Test Plan

### Test 1: Basic Clip Creation & Deletion

**Steps:**
1. Launch Zenith
2. Double-click on "MIDI 1" track lane at around beat 1
3. Verify green clip appears with length ~4 beats
4. Click to select clip (should show white border)
5. Press Delete or Backspace
6. Verify clip disappears
7. Press Ctrl+Z (undo)
8. Verify clip reappears at original position
9. Press Ctrl+Shift+Z (redo)
10. Verify clip disappears again

**Expected:** Create/delete/undo/redo work correctly

---

### Test 2: Move Single Clip (Time)

**Steps:**
1. Create a clip on "Audio 1" track
2. Click and drag clip body to the right (e.g. beat 5)
3. Verify clip follows mouse with live feedback
4. Release mouse
5. Verify clip snaps to grid and stays at new position
6. Press Ctrl+Z
7. Verify clip returns to original position

**Expected:** Move with snap and undo work

---

### Test 3: Move Clip Between Tracks

**Steps:**
1. Create clip on "MIDI 1"
2. Click and drag clip vertically to "Audio 2" track
3. Release mouse
4. Verify clip now appears on "Audio 2" track
5. Press Ctrl+Z
6. Verify clip returns to "MIDI 1"

**Expected:** Cross-track move and undo work

---

### Test 4: Multi-Select & Move

**Steps:**
1. Create 3 clips on different tracks at different times
2. Click first clip (select)
3. Ctrl+click second clip (add to selection)
4. Shift+drag a marquee around all 3 clips
5. Verify all 3 clips show selection border
6. Drag any one clip to the right by ~2 beats
7. Verify all 3 clips move together
8. Release mouse
9. Press Ctrl+Z
10. Verify all 3 clips return to original positions

**Expected:** Multi-select and group move work

---

### Test 5: Resize Clip (Right Edge)

**Steps:**
1. Create clip on "Audio 1" (default 4 beats)
2. Hover over right edge (cursor should change to resize)
3. Drag right edge to extend clip to ~8 beats
4. Release mouse
5. Verify clip length is ~8 beats (snapped)
6. Press Ctrl+Z
7. Verify clip returns to original 4 beats

**Expected:** Right-edge resize and undo work

---

### Test 6: Resize Clip (Left Edge)

**Steps:**
1. Create clip at beat 4, length 4 beats (ends at beat 8)
2. Hover over left edge (resize cursor)
3. Drag left edge to beat 2
4. Release mouse
5. Verify clip now starts at beat 2 and still ends at beat 8 (length ~6 beats)
6. Press Ctrl+Z
7. Verify clip returns to start=4, length=4

**Expected:** Left-edge resize (adjusts start, keeps end) and undo work

---

### Test 7: Duplicate Clips

**Steps:**
1. Create clip on "MIDI 1" at beat 1
2. Select clip
3. Press Ctrl+D
4. Verify duplicate appears immediately after original (at beat 5 if original was 4 beats)
5. Verify duplicate is now selected (original deselected)
6. Press Ctrl+Z
7. Verify duplicate disappears

**Expected:** Duplicate and undo work

---

### Test 8: Marquee Selection

**Steps:**
1. Create 4 clips scattered on different tracks
2. Click empty area to clear selection
3. Hold Shift and drag a rectangle over 2 of the clips
4. Release mouse
5. Verify those 2 clips are selected
6. Press Delete
7. Verify both clips deleted
8. Press Ctrl+Z
9. Verify both clips restored

**Expected:** Marquee select, delete, undo work

---

### Test 9: Zoom & Scroll

**Steps:**
1. Create several clips on different tracks across beats 1-20
2. Ctrl+mousewheel up (zoom in)
3. Verify timeline zooms in around mouse position
4. Ctrl+mousewheel down (zoom out)
5. Verify timeline zooms out
6. Shift+mousewheel (horizontal scroll)
7. Verify timeline scrolls left/right
8. Mousewheel (no modifier, vertical scroll)
9. Verify track lanes scroll up/down
10. Press + key (zoom in)
11. Press - key (zoom out)
12. Verify zoom around center

**Expected:** All zoom/scroll controls work correctly

---

### Test 10: Grid Snap Precision

**Steps:**
1. Create clip at beat 1
2. Drag clip slightly off-grid (e.g. to beat 2.3)
3. Release mouse
4. Verify clip snaps to nearest grid line (beat 2.25 with 1/16 grid)
5. Resize clip to arbitrary length (e.g. 3.7 beats)
6. Release mouse
7. Verify length snaps to grid (e.g. 3.75 beats)

**Expected:** All positions/lengths snap to 1/16 note grid

---

### Test 11: Color Coding

**Steps:**
1. Create clip on "Audio 1" (type = "audio")
2. Verify clip is blue
3. Create clip on "MIDI 1" (type = "midi")
4. Verify clip is green
5. Select both clips
6. Verify both show white selection border and slightly brighter fill

**Expected:** Audio=blue, MIDI=green, selection visible

---

### Test 12: Empty Arranger State

**Steps:**
1. Launch Zenith
2. Delete all 3 default tracks (not implemented yet, so skip this)
3. OR start with empty project
4. Verify Arranger shows empty grid with no crashes

**Expected:** Graceful handling of empty state

---

## Known Limitations

### Out of Scope for Phase 9

**Not Implemented:**
- PianoRoll (MIDI note editing)
- CommandAPI / Wingman integration
- Audio waveform rendering in clips
- MIDI note preview in clips
- Plugin routing
- Clip fade curves
- Crossfades between clips
- Ripple editing
- Track height adjustment
- Snap/grid toggle UI
- Beat/bar/time ruler modes
- Track recording (red-dot workflow)

**Clip Behavior:**
- Overlapping clips are allowed (no collision detection)
- Clips can be placed at negative times (clamped to 0 on commit)
- No clip names editable in UI (must edit ValueTree or add in future)
- Resize currently only affects single clip (not multi-resize)

**Performance:**
- No clip virtualization (all clips always drawn)
- OK for <1000 clips; may need optimization for larger projects

**Grid:**
- Fixed 1/16 note snap (no UI to change)
- No grid-off mode

---

## Files Modified/Created

### New Files
- `zenith-core/include/ArrangerComponent.h` - ArrangerComponent header
- `zenith-core/src/ArrangerComponent.cpp` - ArrangerComponent implementation
- `docs/Phase9_Arranger_ClipEditing_Summary.md` - This document

### Modified Files
- `zenith-core/include/ProjectState.h` - Added clip management APIs
- `zenith-core/src/ProjectState.cpp` - Implemented clip APIs
- `zenith-core/include/MainWindow.h` - Added ArrangerComponent integration
- `zenith-core/src/MainWindow.cpp` - Layout and initialization updates

---

## Build Instructions

```bash
cd /home/user/daw/zenith-core
mkdir -p build
cd build
cmake ..
cmake --build . -j
```

**Run:**
```bash
./ZenithDAW_artefacts/Debug/ZenithDAW   # or Release
```

---

## Next Steps (Future Phases)

**Suggested Priorities:**

1. **PianoRoll Integration** - Double-click MIDI clip → opens piano roll for note editing
2. **Waveform Rendering** - Show audio waveforms in audio clips
3. **Track Controls** - Mute/solo/arm buttons per track
4. **Clip Naming** - Inline text editing for clip names
5. **Grid Settings UI** - Dropdown to select snap resolution (1/4, 1/8, 1/16, etc.)
6. **Undo History Panel** - Show undo stack with transaction names
7. **Clip Colors** - User-configurable clip colors per clip or track
8. **Ripple Edit Mode** - Moving clip shifts all clips to the right
9. **Engine Integration** - Bridge ProjectState clips → engine playback
10. **CommandAPI / Wingman** - Script-based clip operations

---

## Summary

Phase 9 delivers a **fully functional Arranger** with:

✅ **Clip creation** - Double-click to create
✅ **Selection** - Single, multi-select, marquee
✅ **Move** - Time + track with snap and undo
✅ **Resize** - Left/right edges with constraints
✅ **Delete** - Keyboard shortcut with undo
✅ **Duplicate** - Ctrl+D with undo
✅ **Zoom/Scroll** - Horizontal and vertical navigation
✅ **Undo/Redo** - Full integration with UndoManager
✅ **RT-Safe** - Message-thread only, no audio thread changes
✅ **ValueTree Integration** - Instant UI updates on state changes

**The Arranger is now the central workflow hub for Zenith, ready for users to create and arrange music without typing commands.**

---

**End of Phase 9 Summary**
