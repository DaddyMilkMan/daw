# Zenith DAW - Integration Overview

This document describes how all major subsystems integrate to create a complete DAW workflow from recording to export.

## Architecture Layers

```
┌─────────────────────────────────────────────────────────────┐
│                        UI Layer                              │
│  MainWindow, ArrangerView, PianoRollEditor,                 │
│  AutomationLaneComponent                                     │
└─────────────────────────────────────────────────────────────┘
                            ▲ │
                            │ │ (ValueTree listeners, callbacks)
                            │ ▼
┌─────────────────────────────────────────────────────────────┐
│                    ProjectState Layer                        │
│  ValueTree-based state (tracks, clips, notes, automation)   │
│  Undo/Redo support, Serialization to XML                    │
└─────────────────────────────────────────────────────────────┘
                            ▲ │
                            │ │ (Synchronizers: ClipSync, AutomationSync)
                            │ ▼
┌─────────────────────────────────────────────────────────────┐
│                      Engine Layer                            │
│  Audio thread, Tracks, Clips, Playback,                     │
│  RecordingEngine, ExportEngine                               │
└─────────────────────────────────────────────────────────────┘
```

## Data Flow #1: Recording → ArrangerView

**End-to-End Flow:**

```
User clicks Record
    ↓
RecordingManager starts recording
    ↓
Audio/MIDI data captured to Engine Track's Clip
    ↓
ClipSynchronizer detects new clip (timer poll, 30 Hz)
    ↓
ClipSynchronizer creates CLIP node in ProjectState
    ↓
ProjectState fires ValueTree::childAdded event
    ↓
ArrangerView (listener) repaints to show new clip
```

**Key Components:**

1. **RecordingEngine / RecordingManager** (U3 branch)
   - Lives in Engine, writes audio/MIDI to `zenith::Clip` objects
   - Attaches clips to `zenith::Track` containers
   - Thread-safe: records on audio thread, adds clips on message thread

2. **ClipSynchronizer** (`ClipSynchronizer.h/cpp`)
   - Runs on MESSAGE THREAD (timer-based)
   - Polls Engine tracks for new clips (checks clip count)
   - Creates corresponding `ProjectState::ID_CLIP` nodes
   - Converts samples → beats using tempo + sample rate
   - Properties synced: `start`, `length`, `type` (audio/midi)

3. **ProjectState** (`ProjectState.h/cpp`)
   - ValueTree structure:
     ```
     PROJECT
       ├── TRACKS
       │   └── TRACK (id, name, type, volume, pan, mute, solo)
       │       ├── CLIPS
       │       │   └── CLIP (id, type, start, length)
       │       │       └── NOTES (for MIDI, when U3 model merged)
       │       └── AUTOMATION
       │           └── ENVELOPE (param)
       │               └── POINT (id, timeBeats, value)
     ```
   - Fires change notifications via ValueTree::Listener
   - Supports undo/redo for all operations

4. **ArrangerView** (`ArrangerView.h/cpp`)
   - Listens to ProjectState ValueTree changes
   - Paints clips from `TRACKS → TRACK → CLIPS` hierarchy
   - Grid-based timeline (beats as X-axis)
   - Each track displayed as horizontal lane

**When Fully Implemented:**
- RecordingManager in Engine creates clips during record
- ClipSynchronizer polls and syncs to ProjectState
- ArrangerView updates in real-time as clips appear

**Current Stub Behavior:**
- ClipSynchronizer::createClip() creates clips directly in ProjectState
- No actual Engine clip creation yet (waiting for U3 merge)

---

## Data Flow #2: Opening Piano Roll Editor

**End-to-End Flow:**

```
User double-clicks MIDI clip in ArrangerView
    ↓
ArrangerView::mouseDoubleClick detects clip under mouse
    ↓
ArrangerView calls openPianoRollCallback(trackId, clipId)
    ↓
MainComponent::openPianoRoll creates PianoRollEditor window
    ↓
PianoRollEditor reads MIDI notes from ProjectState clip
    ↓
User edits notes (add/move/delete)
    ↓
PianoRollEditor writes to ProjectState NOTES nodes
    ↓
Engine picks up changes and plays modified notes
```

**Key Components:**

1. **ArrangerView** (`ArrangerView.h/cpp`)
   - Implements `mouseDoubleClick()`
   - Calls `findClipAtPosition()` to determine (trackId, clipId)
   - Checks clip type: only opens piano roll for MIDI clips
   - Invokes callback: `openPianoRollCallback(trackId, clipId)`

2. **MainComponent** (`MainWindow.h/cpp`)
   - Sets callback: `arrangerView->setOpenPianoRollCallback(...)`
   - Creates new `PianoRollEditor` window on callback
   - Window is self-owned (deletes itself on close)

3. **PianoRollEditor** (`PianoRollEditor.h/cpp`)
   - DocumentWindow with piano roll content
   - Reads MIDI notes from `ProjectState::CLIP → NOTES` (when U3 MIDI model merged)
   - Displays notes as rectangles on pitch/time grid
   - Writes changes back to ProjectState with undo support

**When Fully Implemented:**
- U3 branch adds `ID_NOTES` child nodes to MIDI clips
- Each NOTE has: `pitch`, `start` (beats), `length` (beats), `velocity`
- PianoRollEditor reads/writes these nodes
- Engine's MIDI playback reflects changes in real-time

**Current Stub Behavior:**
- PianoRollEditor window opens but shows placeholder
- NOTES structure not yet implemented (waiting for U3 merge)
- Click events log intent to add notes

---

## Data Flow #3: Automation Display & Editing

**End-to-End Flow:**

```
User clicks "Show Automation" button for track in ArrangerView
    ↓
MainComponent toggles ArrangerView::setTrackAutomationVisible(trackId, true)
    ↓
ArrangerView creates AutomationLaneComponent for track
    ↓
AutomationLaneComponent reads ENVELOPE from ProjectState
    ↓
User clicks to add automation point
    ↓
AutomationLaneComponent calls ProjectState::addAutomationPoint()
    ↓
TrackAutomationSynchronizer samples automation curve (60 Hz)
    ↓
TrackAutomationSynchronizer writes to Track volume/pan/mute atomics
    ↓
Audio thread reads atomics and applies automation
```

**Key Components:**

1. **MainComponent** (`MainWindow.h/cpp`)
   - Creates per-track automation toggle buttons ("A" button per track)
   - Buttons call: `arrangerView->setTrackAutomationVisible(trackId, !visible)`
   - Buttons positioned in left sidebar, aligned with tracks

2. **ArrangerView** (`ArrangerView.h/cpp`)
   - Method: `setTrackAutomationVisible(trackId, show)`
   - Creates/destroys `AutomationLaneComponent` instances
   - Positions automation lanes below their parent track
   - Adjusts track spacing when automation visible

3. **AutomationLaneComponent** (`ArrangerView.h/cpp`)
   - Displays automation curve for one parameter (volume/pan/mute)
   - Paints points and interpolated curve from ProjectState ENVELOPE
   - `mouseDown()` adds new point via `ProjectState::addAutomationPoint()`
   - `mouseDrag()` moves existing points via `ProjectState::moveAutomationPoint()`
   - All edits go through ProjectState with undo support

4. **ProjectState** (`ProjectState.h/cpp`)
   - Methods:
     - `getOrCreateAutomationEnvelope(trackId, paramId)`
     - `addAutomationPoint(trackId, paramId, timeBeats, value, actionName)`
     - `moveAutomationPoint(...)`
     - `deleteAutomationPoint(...)`
   - ENVELOPE structure stores sorted list of POINTs
   - All changes are undoable

5. **TrackAutomationSynchronizer** (`TrackAutomationSynchronizer.h/cpp`)
   - Runs timer on MESSAGE THREAD (60 Hz)
   - Samples automation envelopes at current playback position
   - Interpolates between points to get current value
   - Writes to `zenith::Track` atomics:
     - `track->setVolume(sampledValue)`
     - `track->setPan(sampledValue)`
     - `track->setMuted(sampledValue >= 0.5)`
   - Audio thread reads atomics (lock-free)

**Thread Safety:**
- UI edits automation: MESSAGE THREAD
- TrackAutomationSynchronizer samples: MESSAGE THREAD
- Synchronizer writes to atomics: MESSAGE THREAD
- Audio thread reads atomics: AUDIO THREAD (lock-free)

**Current Implementation:**
- Full automation pipeline is functional (Phase 13)
- UI stubs in place for editing automation curves
- Ready for full UI integration when branches merge

---

## Data Flow #4: Export to WAV

**End-to-End Flow:**

```
User selects "Export → Mixdown to WAV"
    ↓
ExportEngine reads ProjectState for all tracks/clips/automation
    ↓
ExportEngine renders offline at project sample rate
    ↓
For each audio block:
  - Read clip audio data
  - Apply automation (volume/pan/mute)
  - Mix all tracks
    ↓
ExportEngine writes WAV file
```

**Key Components:**

1. **ExportEngine** (U3.2 branch)
   - Reads ProjectState to know what to render
   - Creates offline rendering graph
   - Processes in blocks (non-real-time)
   - Writes to WAV file using JUCE AudioFormatWriter

2. **ProjectState** (`ProjectState.h/cpp`)
   - Provides complete state snapshot for export
   - ExportEngine reads:
     - All tracks (type, volume, pan, mute)
     - All clips (start, length, audio data, MIDI notes)
     - All automation envelopes
     - Project tempo, time signature, sample rate

3. **Engine** (`Engine.h/cpp`)
   - Provides sample rate for offline rendering
   - ExportEngine may use Engine's track container directly
   - Or create temporary offline rendering graph

**When Fully Implemented:**
- ExportEngine::exportProjectToWav(file, startBeats, lengthBeats)
- Reads ProjectState to configure offline graph
- Renders at 1x or faster (offline = no real-time constraints)
- Bounces to stereo WAV file

**Current Stub Behavior:**
- ExportEngine not yet implemented (waiting for U3.2 merge)
- Integration point ready: ProjectState provides all data needed

---

## Component Integration Map

### MainWindow Lifecycle

```cpp
MainWindow::MainWindow()
{
    // 1. Create Engine (audio thread)
    engine = std::make_unique<Engine>();

    // 2. Create ProjectState (data model)
    projectState = std::make_unique<ProjectState>();

    // 3. Connect Engine ↔ ProjectState (automation sync)
    engine->setProjectState(projectState.get());
    // → Creates TrackAutomationSynchronizer

    // 4. Create ClipSynchronizer (recording → state)
    clipSynchronizer = std::make_unique<ClipSynchronizer>(*projectState, *engine);

    // 5. Create UI
    mainComponent = std::make_unique<MainComponent>(*engine, *projectState);
    // → Creates ArrangerView
    // → Sets up piano roll callback
    // → Creates automation toggle buttons

    // 6. Initialize Engine (starts audio device)
    engine->initialize();

    // 7. Start synchronizers (when needed)
    // clipSynchronizer->start(30);  // For recording
    // automationSynchronizer->start(60);  // For playback (in Engine::play)
}
```

### Data Ownership

- **MainWindow owns:**
  - `Engine` (unique_ptr)
  - `ProjectState` (unique_ptr)
  - `ClipSynchronizer` (unique_ptr)
  - `MainComponent` (unique_ptr)

- **Engine owns:**
  - `TrackAutomationSynchronizer` (unique_ptr, created in setProjectState)
  - `std::vector<zenith::Track>` (track container)
  - RecordingManager (when U3 merged)

- **MainComponent owns:**
  - `ArrangerView` (unique_ptr)
  - Automation toggle buttons (map of unique_ptrs)

- **ArrangerView owns:**
  - `AutomationLaneComponent` instances (map of unique_ptrs, per visible track)

- **PianoRollEditor:**
  - Self-owned DocumentWindow (deletes self on close)

### Synchronizer Responsibilities

| Synchronizer | Direction | Thread | Frequency | Purpose |
|--------------|-----------|--------|-----------|---------|
| **ClipSynchronizer** | Engine → ProjectState | Message | 30 Hz | Sync recorded clips to state |
| **TrackAutomationSynchronizer** | ProjectState → Engine | Message | 60 Hz | Sample automation curves → track atomics |

### ValueTree Listener Pattern

```cpp
// ArrangerView listens to entire ProjectState tree
ArrangerView::ArrangerView(ProjectState& ps)
{
    projectState.getState().addListener(this);
    // Receives all track/clip/automation changes
}

// When ProjectState changes:
ProjectState::addAutomationPoint(...)
{
    envelope.addChild(point, index, &undoManager);
    // → Fires valueTreeChildAdded(envelope, point)
    // → ArrangerView receives event
    // → ArrangerView::repaint()
}
```

---

## Threading Model

### Audio Thread (Real-Time Safe)
- `Engine::audioDeviceIOCallbackWithContext()`
- Reads: `Track` atomics (volume, pan, mute)
- Reads: `Clip` position, audio buffers (via lock-free mechanisms)
- **Never allocates, never locks, never calls DBG**

### Message Thread
- All UI components
- ProjectState edits (ValueTree operations)
- Synchronizer timers (ClipSynchronizer, TrackAutomationSynchronizer)
- Engine control (play/stop/record)

### Thread-Safe Communication
- **Atomics:** `Track::volume`, `Track::pan`, `Track::muted`
- **Message thread writes, audio thread reads**
- **No locks required**

---

## Future Integration Points

### When U3 (Recording) Branch Merges:
- [ ] RecordingManager integrated into Engine
- [ ] ClipSynchronizer polls Engine for new clips
- [ ] Clips appear in ArrangerView during recording
- [ ] MIDI note model added to ProjectState clips
- [ ] PianoRollEditor reads/writes NOTES nodes

### When U3.2 (Export) Branch Merges:
- [ ] ExportEngine::exportProjectToWav() implemented
- [ ] Reads ProjectState for offline rendering
- [ ] Bounces to WAV file with automation applied

### When Plugin Hosting (Phase 2) Merges:
- [ ] Track plugin chain integration
- [ ] Plugin automation parameters in ProjectState
- [ ] AutomationLaneComponent supports plugin parameters

### When Mixer UI Merges:
- [ ] MixerComponent displays tracks
- [ ] Faders control ProjectState volume/pan
- [ ] TrackAutomationSynchronizer syncs both directions

---

## Build Integration

### CMakeLists.txt Changes Required

When building with integration components, add:

```cmake
# Integration sources
set(INTEGRATION_SOURCES
    src/ClipSynchronizer.cpp
    src/ArrangerView.cpp
    src/PianoRollEditor.cpp
)

# Integration headers
set(INTEGRATION_HEADERS
    include/ClipSynchronizer.h
    include/ArrangerView.h
    include/PianoRollEditor.h
)

# Add to target
target_sources(ZenithDAW PRIVATE
    ${INTEGRATION_SOURCES}
    ${INTEGRATION_HEADERS}
)
```

### Dependencies
- JUCE 8.0.9 (already present)
- C++20 (already enabled)
- No additional external dependencies

---

## Testing Strategy

### Unit Tests
- **ProjectState:** Track/clip/automation CRUD operations
- **ClipSynchronizer:** Beats ↔ samples conversion
- **TrackAutomationSynchronizer:** Envelope sampling

### Integration Tests
1. **Recording Flow:**
   - Create clip in Engine
   - Verify ClipSynchronizer syncs to ProjectState
   - Verify ArrangerView displays clip

2. **Piano Roll Flow:**
   - Create MIDI clip in ProjectState
   - Double-click in ArrangerView
   - Verify PianoRollEditor opens

3. **Automation Flow:**
   - Add automation points via UI
   - Verify points stored in ProjectState
   - Verify TrackAutomationSynchronizer samples correctly
   - Verify audio output reflects automation

4. **Export Flow:**
   - Populate ProjectState with clips/automation
   - Call ExportEngine::exportProjectToWav()
   - Verify WAV file contains correct audio

### UI Tests
- ArrangerView clip rendering
- AutomationLaneComponent curve drawing
- PianoRollEditor note editing
- Automation toggle buttons

---

## Summary: End-to-End Pipeline

```
┌──────────────┐
│ User Records │
└──────┬───────┘
       │
       ▼
┌──────────────────────┐
│ RecordingEngine      │ (U3 branch)
│ Creates zenith::Clip │
└──────┬───────────────┘
       │
       ▼
┌──────────────────────┐
│ ClipSynchronizer     │ (30 Hz timer)
│ Engine → ProjectState│
└──────┬───────────────┘
       │
       ▼
┌──────────────────────┐
│ ProjectState         │ (ValueTree)
│ Stores clips/notes/  │
│ automation           │
└──────┬───────────────┘
       │
       ├──────────────────────────┐
       │                          │
       ▼                          ▼
┌──────────────────┐   ┌──────────────────────┐
│ ArrangerView     │   │ TrackAutomation      │
│ Displays clips   │   │ Synchronizer (60 Hz) │
└──────┬───────────┘   │ ProjectState → Track │
       │               │ atomics              │
       │               └──────┬───────────────┘
       ▼                      │
┌──────────────────┐          │
│ User double-     │          │
│ clicks MIDI clip │          │
└──────┬───────────┘          │
       │                      │
       ▼                      ▼
┌──────────────────┐   ┌──────────────────┐
│ PianoRollEditor  │   │ Audio Thread     │
│ Edits MIDI notes │   │ Reads atomics    │
└──────────────────┘   │ Applies volume/  │
                       │ pan/mute         │
                       └──────────────────┘
```

This integration architecture provides:
- **Separation of concerns:** UI, State, Engine are decoupled
- **Thread safety:** Message thread for state, audio thread for playback
- **Undo/redo:** All user edits go through ProjectState
- **Real-time automation:** Lock-free atomics for audio thread
- **Extensibility:** Easy to add new UI views or export formats

When all branches (U3, U3.2, UI components) are merged, this creates a complete DAW workflow from recording through editing to export.
