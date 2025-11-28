# Build Notes: Arranger + Piano Roll Integration

## System Requirements

JUCE 8.0.9 requires the following system libraries on Linux:
- X11 development headers: `libx11-dev`, `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`
- ALSA development headers: `libasound2-dev`
- FreeType: `libfreetype6-dev`

On a properly configured system, build with:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

## What Was Implemented

### 1. ProjectState Enhancements
- Added beat-based clip properties (`startBeats`, `lengthBeats`)
- Added NOTES/NOTE support for MIDI notes
- API methods:
  - `addClip()`, `moveClip()`, `deleteClip()`
  - `addNote()`, `moveNote()`, `deleteNote()`
  - All operations use UndoManager for undo/redo support

### 2. UI Components Created

#### TimelineRuler (`include/ui/TimelineRuler.h`)
- Displays beat markers and measure numbers
- Horizontal grid lines for beat divisions
- Configurable zoom (pixels per beat)

#### ClipComponent (`include/ui/ClipComponent.h`)
- Visual representation of clips on timeline
- Different colors for audio vs MIDI clips
- Mouse interaction for selection and dragging

#### ArrangerView (`include/ui/ArrangerView.h`)
- Main timeline view with track lanes
- Integrates TimelineRuler
- Displays clips from ProjectState
- Beat grid background
- Features:
  - Double-click empty space to create clip
  - Double-click clip to open editor (stubbed)
  - Listens to ProjectState changes and updates UI
  - Horizontal scroll and zoom

#### PianoRollEditor (`include/ui/PianoRollEditor.h`)
- MIDI note editor with piano roll grid
- Piano keyboard on left showing note names
- Beat grid
- Features:
  - Click to add note (snaps to 1/4 beat grid)
  - Drag to move/transpose notes
  - Double-click to delete note
  - All edits go through ProjectState with undo support

### 3. MainComponent Integration
- ArrangerView now fills center of main window
- Added test buttons:
  - "Add MIDI Track" - creates a new MIDI track
  - "Test Piano Roll" - opens piano roll for testing
- Piano roll opens in separate window

## Usage

1. **Add a track**: Click "Add MIDI Track" button
2. **Create a clip**: Double-click on a track lane in the arranger
3. **Open piano roll**: Click "Test Piano Roll" button
4. **Edit notes**:
   - Click in piano roll to add note
   - Drag note to move/transpose
   - Double-click note to delete
5. **Undo/Redo**: Use Cmd+Z / Cmd+Shift+Z (macOS) or Ctrl+Z / Ctrl+Shift+Z (Linux/Windows)

## Limitations / TODOs

1. **No tempo map**: All timing is in beats, no conversion to samples yet
2. **No audio playback**: Clips don't trigger audio yet
3. **No clip dragging in arranger**: Visual feedback only, doesn't update ProjectState
4. **No automation UI**: AutomationLaneComponent not implemented (automation data exists in ProjectState)
5. **No clip splitting/trimming**: Can only move entire clips
6. **No note velocity editing**: Fixed at 100
7. **No MIDI playback**: Notes are stored but not rendered to audio
8. **Beat-to-sample sync**: UI operates in beats, engine in samples - no sync layer yet

## Architecture Notes

- **Beat-based UI**: All UI components work in beats as the canonical unit
- **ValueTree-driven**: UI components listen to ProjectState ValueTree and update automatically
- **Undo-friendly**: All edits go through UndoManager
- **No engine coupling**: UI components don't touch the audio engine directly
- **Future tempo map**: When added, will just need beat-to-sample conversion in a sync layer
