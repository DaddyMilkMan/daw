# Phase 4: Timeline/Arranger + Piano Roll - Implementation Summary

**Date:** 2025-11-14
**Status:** ✅ Complete

---

## Overview

This document summarizes the timeline/arranger view and piano roll MIDI editor implementation for Zenith DAW Phase 4.

## Components Implemented

### 1. ArrangerComponent (Timeline/Arranger View)

**Location:** `Source/ui/ArrangerComponent.{h,cpp}`

**Responsibilities:**
- Display tracks as horizontal lanes
- Render clips as rectangles on timeline
- Draw time ruler with bars/beats grid
- Show playhead during playback
- Handle clip selection and interaction
- Open piano roll on MIDI clip double-click

**Features:**
- **Time mapping:** Beats-based (tempo = 120 BPM default)
- **ClipVisual struct:** Lightweight hit-testing without Component overhead (scalable to 100s of clips)
- **View parameters:**
  - `pixelsPerBeat`: Horizontal zoom level (default 40)
  - `trackHeight`: Height of each track lane (default 80px)
  - `rulerHeight`: Time ruler height (30px)
- **Grid rendering:**
  - Vertical lines per beat (thicker every 4 beats = 1 bar)
  - Horizontal lines per track
  - Audio clips: blue
  - MIDI clips: green

**Interaction:**
- Click clip → selects (yellow border)
- Double-click MIDI clip → opens piano roll
- Drag clip → moves horizontally (stubbed - TODO: wire to Engine/ProjectState)

**RT-Safety:**
- Reads Engine tracks/clips on message thread
- No audio thread access
- Timer-driven repaint for playhead (60 FPS)

### 2. PianoRollComponent (MIDI Editor)

**Location:** `Source/ui/PianoRollComponent.{h,cpp}`

**Responsibilities:**
- Display piano keys (88-key range: A0-C8)
- Draw note grid (time × pitch)
- Render MIDI notes as rectangles
- Handle note editing (create, move, delete)

**Features:**
- **Layout:**
  - Left panel: Piano keys (60px wide)
  - Main area: Note grid with bars/beats
- **View parameters:**
  - `pixelsPerBeat`: Horizontal zoom (default 40)
  - `noteHeight`: Height per semitone (default 12px)
  - `gridResolution`: Snap resolution (default 0.25 = 1/16 note)
- **Note rendering:**
  - Green rectangles for notes
  - Yellow for selected note
  - Black borders
- **Grid:**
  - Vertical lines per beat (thicker every 4 beats)
  - Horizontal lines per semitone
  - White/black key coloring on piano panel

**Interaction:**
- Click empty space → creates note (default 1/4 note length, velocity 100)
- Click note → selects
- Drag note → moves (pitch + time) with grid snapping
- Delete/Backspace key → deletes selected note
- All edits immediately modify clip's MidiMessageSequence

**Data Access:**
- Direct read/write to `Clip::getMidiSequence()` / `setMidiSequence()`
- Changes are immediate (no undo/redo yet - TODO for future)

### 3. PianoRollWindow

**Location:** `Source/ui/PianoRollComponent.{h,cpp}` (bottom of file)

**Responsibilities:**
- DocumentWindow wrapper for PianoRollComponent
- Similar pattern to PluginEditorWindow
- Self-deleting on close

**Features:**
- Resizable window (1000×600 default)
- Window title shows clip name
- Opens centered on screen

### 4. MainWindow/MainComponent Integration

**Modified Files:** `include/MainWindow.h`, `src/MainWindow.cpp`

**Changes:**
- Added `std::unique_ptr<ArrangerComponent> arrangerComponent` to MainComponent
- Updated layout:
  - Top bar: Status, CPU, track count
  - **Middle (new):** ArrangerComponent fills remaining space
  - Bottom bar: Transport buttons
- Removed welcome screen
- Updated status label to "Phase 4: Timeline + Piano Roll"

### 5. Engine Enhancements

**Modified Files:** `include/Engine.h`

**Changes:**
- Added `getPlaybackPosition()` method to expose playhead position for UI

---

## Architecture

### Time Mapping

**Beats-based system:**
- 1 beat = 1 quarter note
- Tempo = 120 BPM (default, can be extended to read from ProjectState)
- Formula: `beats = samples / (sampleRate * 60.0 / tempo)`

**Helper functions:**
```cpp
// ArrangerComponent and PianoRollComponent both have:
float beatsToPixels(double beats);
double pixelsToBeats(float pixels);
int64_t beatsToSamples(double beats);
double samplesToBeats(int64_t samples);
```

### ClipVisual Structure (Arranger)

```cpp
struct ClipVisual {
    zenith::Track* track;
    zenith::Track::Clip* clip;
    juce::Rectangle<float> bounds;
    bool isMidi;
    int trackIndex;
};
```

- **Purpose:** Fast hit-testing without creating JUCE Components per clip
- **Built in:** `updateClipCache()` from Engine tracks
- **Used in:** `hitTestClip()` for mouse interaction

### NoteVisual Structure (Piano Roll)

```cpp
struct NoteVisual {
    int noteNumber;         // MIDI note (0-127)
    double startTime;       // seconds
    double duration;        // seconds
    int velocity;           // 0-127
    juce::Rectangle<float> bounds;
};
```

- **Purpose:** Fast hit-testing for MIDI notes
- **Built in:** `updateNoteCache()` from clip's MidiMessageSequence
- **Used in:** `hitTestNote()` for mouse interaction

---

## Data Flow

### Arranger → Engine

```
ArrangerComponent
  ├── Reads: Engine::tracks() (message thread)
  ├── Reads: Engine::getPlaybackPosition() (for playhead)
  └── Writes: None (read-only for MVP)
```

### Piano Roll → Clip → Engine

```
PianoRollComponent
  ├── Reads: Clip::getMidiSequence()
  ├── Writes: Clip::setMidiSequence() (creates, moves, deletes notes)
  └── Clip is owned by Track, which is owned by Engine
```

**Note:** Changes are immediate - no undo/redo system yet (future work)

---

## User Workflows

### Viewing Timeline

1. Launch Zenith DAW
2. Create/load tracks (via Engine debugging or future UI)
3. See tracks as horizontal lanes
4. See clips as colored rectangles (blue = audio, green = MIDI)
5. Playhead moves during playback (white vertical line)

### Selecting Clips

1. Click any clip → yellow selection border
2. Click empty space → deselects

### Editing MIDI Notes

1. Double-click MIDI clip in timeline → piano roll window opens
2. **Create note:** Click empty grid space
3. **Move note:** Drag note to new pitch/time (snaps to grid)
4. **Delete note:** Select note, press Delete or Backspace key
5. Close window when done
6. Changes are immediately saved to clip

---

## Manual Test Plan

### Test 1: Arranger View Basic Display

**Steps:**
1. Build and run Zenith
2. Create test tracks with some clips (use Engine debug methods if needed)
3. Verify:
   - Timeline shows tracks as lanes
   - Clips render as rectangles
   - Time ruler shows bar numbers
   - Grid lines visible

**Expected:**
- Clean visual layout
- No rendering glitches
- Smooth painting at 60 FPS

### Test 2: Clip Selection

**Steps:**
1. Click various clips in timeline
2. Verify selected clip has yellow border
3. Click empty space → selection clears

**Expected:**
- Selection feedback immediate
- Only one clip selected at a time

### Test 3: Playhead Animation

**Steps:**
1. Press Play button
2. Watch playhead (white vertical line) move across timeline
3. Press Stop

**Expected:**
- Playhead moves smoothly
- Playhead position matches audio playback
- Stops when Stop pressed

### Test 4: Open Piano Roll

**Steps:**
1. Create MIDI clip with some notes
2. Double-click MIDI clip in timeline
3. Piano roll window opens

**Expected:**
- Window appears centered
- Shows piano keys on left
- Shows note grid
- Existing notes render correctly

### Test 5: Create MIDI Note

**Steps:**
1. Open piano roll for MIDI clip
2. Click empty grid space
3. Note appears as green rectangle

**Expected:**
- Note created at clicked position
- Default length (1/4 note)
- Snaps to grid

### Test 6: Move MIDI Note

**Steps:**
1. Select existing note (click it → turns yellow)
2. Drag to new position (different pitch/time)
3. Release mouse

**Expected:**
- Note moves smoothly
- Snaps to grid on release
- New position saved to clip

### Test 7: Delete MIDI Note

**Steps:**
1. Select note (click it)
2. Press Delete or Backspace key

**Expected:**
- Note disappears
- Removed from clip's MIDI sequence

### Test 8: Multiple Piano Roll Windows

**Steps:**
1. Open piano roll for MIDI clip A
2. Open piano roll for MIDI clip B (double-click different clip)
3. Verify both windows open simultaneously
4. Edit notes in both
5. Close windows

**Expected:**
- Multiple windows can be open
- Each edits its own clip
- No interference between windows
- Windows clean up on close

---

## Known Limitations / Future Work

### Current Limitations

1. **No Undo/Redo:**
   - MIDI edits are immediate and irreversible
   - **Future:** Implement undo/redo using juce::UndoManager

2. **Fixed Tempo:**
   - Hardcoded to 120 BPM
   - **Future:** Read tempo from ProjectState or Engine

3. **No Zoom UI:**
   - pixelsPerBeat and noteHeight are hardcoded
   - **Future:** Add zoom controls (mouse wheel, zoom buttons)

4. **No Scrolling:**
   - Arranger and piano roll don't scroll yet
   - **Future:** Add juce::Viewport or custom scrolling

5. **Clip Movement Stubbed:**
   - Arranger allows dragging clips but doesn't persist changes
   - **Future:** Wire to ProjectState or Engine to actually move clips

6. **No Audio Waveform Display:**
   - Audio clips are just blue rectangles
   - **Future:** Render waveform thumbnails

7. **No Velocity Editing:**
   - Piano roll shows notes but velocity is fixed at 100
   - **Future:** Add velocity lane below note grid

8. **No Note Resize:**
   - Can move notes but not change duration
   - **Future:** Add resize handles on note edges

9. **No Track Headers:**
   - No track names, mute/solo buttons in arranger
   - **Future:** Add TrackListPanel on left side

10. **No Automation Lanes:**
    - No visual automation editing yet
    - **Future:** Add automation lane view below clips

---

## Files Created

**New UI Components:**
- `Source/ui/ArrangerComponent.h`
- `Source/ui/ArrangerComponent.cpp`
- `Source/ui/PianoRollComponent.h`
- `Source/ui/PianoRollComponent.cpp`

**Documentation:**
- `docs/Phase4_Timeline_PianoRoll_Summary.md`

**Modified Files:**
- `include/MainWindow.h` (added ArrangerComponent member)
- `src/MainWindow.cpp` (integrated arranger into layout)
- `include/Engine.h` (added getPlaybackPosition() method)
- `CMakeLists.txt` (added new source files)

---

## Build Configuration

**CMakeLists.txt Changes:**
- Added `Source/ui/ArrangerComponent.{h,cpp}`
- Added `Source/ui/PianoRollComponent.{h,cpp}`

**No additional dependencies** - all based on existing JUCE modules.

---

## Design Decisions

1. **No Component Per Clip:**
   - Used ClipVisual struct + manual painting instead
   - **Reason:** Scalability - 100s of clips would create 100s of Components (expensive)
   - **Tradeoff:** Manual hit-testing, but much better performance

2. **Direct Clip Modification:**
   - Piano roll directly modifies Clip::setMidiSequence()
   - **Reason:** Simplest MVP, immediate feedback
   - **Tradeoff:** No undo/redo (future work)

3. **Beats-Based Time:**
   - Used beats (quarter notes) instead of seconds
   - **Reason:** More musical, aligns with bars/beats grid
   - **Tradeoff:** Requires tempo for conversion (currently hardcoded)

4. **Fixed Grid Snap:**
   - 1/16 note resolution
   - **Reason:** Good default for most music
   - **Tradeoff:** Not adjustable yet (future work)

5. **Self-Deleting Windows:**
   - PianoRollWindow deletes itself on close (like PluginEditorWindow)
   - **Reason:** Simple memory management for MVP
   - **Tradeoff:** No global window manager (could add later)

---

## Next Steps (Phase 5+)

1. **Undo/Redo System:**
   - Integrate juce::UndoManager for all edits
   - Track-level and project-level undo

2. **Tempo/Time Signature:**
   - Read from ProjectState
   - UI controls for changing

3. **Zoom & Scroll:**
   - Mouse wheel zoom (horizontal + vertical)
   - Scrollbars or custom drag-to-scroll
   - Zoom to fit, zoom to selection

4. **Track Headers Panel:**
   - Track names
   - Arm/mute/solo/record buttons
   - Volume/pan controls
   - Plugin insert slots

5. **Audio Waveforms:**
   - Render waveform thumbnails in audio clips
   - Lazy loading/caching for large files

6. **Automation:**
   - Volume/pan/plugin parameter automation lanes
   - Breakpoint editing (create/move/delete points)

7. **Advanced MIDI Editing:**
   - Velocity lane
   - Note resize (change duration)
   - Quantize
   - Transpose
   - CC/pitch bend lanes

8. **Clip Editing:**
   - Actually move clips (wire to ProjectState)
   - Resize clips (trim)
   - Split/merge clips
   - Copy/paste

9. **Wingman Integration:**
   - Use existing Ableton command schema
   - Map to Zenith Track/Clip/Plugin API
   - Natural language → DAW actions

---

## Conclusion

✅ **Phase 4 Complete:** Zenith DAW now has a functional timeline/arranger view and piano roll MIDI editor:
- Visual representation of tracks and clips
- MIDI note editing with create/move/delete
- Playhead animation during playback
- Clean, scalable architecture

The implementation provides a solid foundation for future enhancements while maintaining RT-safety and good performance.

---

**Next:** Phase 5 - Wingman Integration + Advanced Editing Features
