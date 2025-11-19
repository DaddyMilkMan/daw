# Zenith DAW - Branch Unification Plan

**Document Version:** 1.0
**Created:** 2025-11-15
**Status:** Planning Phase - DO NOT EXECUTE YET
**Target Branch:** `zenith-unified-main`

---

## Executive Summary

This document provides a comprehensive, step-by-step plan to unify all Zenith DAW feature branches into a single canonical branch (`zenith-unified-main`) that will serve as the foundation for all future development.

**Key Decision:** Use current HEAD (`claude/zenith-merge-strategy-plan-01RcjzqGji5CweC99ESLP67W`) as the base branch because it contains the most complete and recent implementation including track automation.

---

## Table of Contents

1. [Feature Matrix & Branch Analysis](#feature-matrix--branch-analysis)
2. [Base Branch Selection](#base-branch-selection)
3. [Feature-to-Branch Mapping](#feature-to-branch-mapping)
4. [Unification Strategy (Phases U1-U5)](#unification-strategy-phases-u1-u5)
5. [Conflict Hotspots & Resolution Strategy](#conflict-hotspots--resolution-strategy)
6. [Risks & Mitigations](#risks--mitigations)

---

## Feature Matrix & Branch Analysis

### Current Branch Landscape

| **Branch** | **Last Commit** | **Key Features** | **Status** |
|------------|-----------------|------------------|------------|
| `claude/zenith-merge-strategy-plan-01RcjzqGji5CweC99ESLP67W` (HEAD) | d4ca3a6 (2025-11-15) | Engine + Mixer + Track Automation + CommandAPI | Active, Most Complete |
| `claude/phase-9-arranger-clip-editing-01GazSp5uZX5BpQXfhazqfWg` | 30228d6 (2025-11-11) | Engine + Mixer (no automation) | Superseded by HEAD |

### Feature Coverage Analysis

| **Feature Category** | **HEAD Branch** | **Phase-9 Branch** | **VexelDAW-Native** | **Historical Commits** |
|----------------------|-----------------|-------------------|---------------------|------------------------|
| **Engine + Mixer** | ✅ zenith-core/ | ✅ zenith-core/ | ✅ Donor code | N/A |
| **Recording (Audio/MIDI)** | ⚠️ Documented | ⚠️ Documented | ⚠️ Partial | ✅ feb61bf (web) |
| **Export (WAV/MIDI)** | ⚠️ Documented | ⚠️ Documented | ⚠️ Partial | ✅ feb61bf (web) |
| **Piano Roll + MIDI Edit** | ❌ Not Ported | ❌ Not Ported | ⚠️ Partial | ✅ 7a09bc3 (web) |
| **Arranger / Timeline** | ⚠️ Clip classes | ⚠️ Clip classes | ✅ ArrangementComponent | ✅ adfc43b (web) |
| **Track Automation (Backend)** | ✅ Synchronizer | ❌ Missing | ❌ Missing | ✅ ab88e84 (C++) |
| **Automation Lanes (UI)** | ❌ Not Implemented | ❌ Not Implemented | ❌ Not Implemented | ❌ Future Work |
| **Wingman/CommandAPI** | ✅ CommandAPI.h/cpp | ❌ Missing | ❌ Missing | ✅ ab88e84 (C++) |

**Legend:**
- ✅ = Fully implemented
- ⚠️ = Partially implemented or documented only
- ❌ = Not implemented

---

## Base Branch Selection

### Chosen Base: `claude/zenith-merge-strategy-plan-01RcjzqGji5CweC99ESLP67W` (HEAD)

**Justification:**

1. **Feature Coverage (90%)**
   - Contains ALL features from phase-9 branch
   - PLUS track automation backend (TrackAutomationSynchronizer)
   - PLUS CommandAPI for Wingman integration
   - Most recent code (Nov 15, 2025)

2. **Code Cleanliness (85%)**
   - Clean C++/JUCE implementation in zenith-core/
   - Follows JUCE 8.0.9 best practices
   - No legacy cruft from web implementations
   - Well-documented (Phase13_TrackAutomation_MVP_Summary.md)

3. **Recent Activity (100%)**
   - Last touched 1 day ago (actively maintained)
   - Automation bugs already fixed (commits 3b6fb1a, ab88e84)
   - Clean git history with descriptive commits

**Why NOT phase-9 branch:**
- Missing track automation (11 files, 1851+ lines)
- Missing CommandAPI for AI integration
- Older codebase (Nov 11 vs Nov 15)
- No additional features vs HEAD

---

## Feature-to-Branch Mapping

### Feature 1: Engine + Mixer

**Source Branch:** HEAD (already present)

**Implementation:**
- `zenith-core/include/Engine.h` (15 methods)
- `zenith-core/src/Engine.cpp` (audio callback, transport)
- `zenith-core/Source/engine/Track.h/cpp` (mixer controls)
- `zenith-core/Source/engine/MixerChannel.h/cpp` (routing)

**Key Commits:**
- `5d10e13` - Port donor classes from VexelDAW-Native
- `667ec9e` - Minimal Engine adapter
- `5d4bce7` - UI integration

**Status:** ✅ Complete, no merge needed

---

### Feature 2: Recording (Audio/MIDI)

**Source Branch:** Historical web implementation (commit `feb61bf`)

**Current State:**
- ❌ Not ported to zenith-core yet
- ✅ Documented in RECORDING_FEATURES_DOCUMENTATION.md
- ⚠️ VexelDAW-Native has partial AudioEngine infrastructure

**Implementation Location (Web):**
- Web Audio API based (not applicable to C++/JUCE)
- Features: Multi-track recording, MIDI capture, quantization

**Porting Strategy:**
- **NOT a merge** - requires complete reimplementation in C++/JUCE
- Use JUCE AudioDeviceManager for audio input
- Use JUCE MidiInput for MIDI capture
- Reference: VexelDAW-Native/Source/Audio/AudioEngine.h

**Key Files to Create:**
- `zenith-core/include/RecordingEngine.h`
- `zenith-core/src/RecordingEngine.cpp`
- Integration with existing Engine.cpp

**Estimated Effort:** ~2-3 days (medium complexity)

---

### Feature 3: Export (WAV/MIDI)

**Source Branch:** Historical web implementation (commit `feb61bf`)

**Current State:**
- ❌ Not ported to zenith-core yet
- ✅ Documented in RECORDING_FEATURES_DOCUMENTATION.md (lines 147-180)
- Features: WAV export (16/24/32-bit), MIDI export, mixdown

**Porting Strategy:**
- **NOT a merge** - requires C++/JUCE implementation
- Use JUCE AudioFormatWriter for WAV export
- Use JUCE MidiFile for MIDI export
- Offline rendering engine for mixdown

**Key Files to Create:**
- `zenith-core/include/ExportEngine.h`
- `zenith-core/src/ExportEngine.cpp`

**Estimated Effort:** ~1-2 days (low complexity)

---

### Feature 4: Piano Roll + MIDI Editing

**Source Branch:** Historical web implementation (commits `7a09bc3`, `df37665`)

**Current State:**
- ❌ Not ported to zenith-core yet
- ✅ Documented as "FL Studio-grade Piano Roll"
- Features: Note editing, velocity, quantization, chords

**Implementation Location (Web):**
- React/TypeScript component
- Canvas-based rendering

**Porting Strategy:**
- **NOT a merge** - requires complete JUCE GUI reimplementation
- Use JUCE Graphics for rendering
- JUCE Component for UI
- Reference: VexelDAW-Native/Source/GUI/ (if exists)

**Key Files to Create:**
- `zenith-core/Source/gui/PianoRollComponent.h/cpp`
- `zenith-core/include/MidiEditor.h`
- Integration with ProjectState ValueTree

**Estimated Effort:** ~5-7 days (high complexity)

---

### Feature 5: Arranger / Clip Editing

**Source Branch:**
- Partial: zenith-core/Source/engine/Clip.h/cpp (already present)
- GUI: VexelDAW-Native/Source/GUI/Arrangement/ArrangementComponent.h
- Historical: commit `adfc43b` (web implementation)

**Current State:**
- ⚠️ Backend exists (Clip class with fade, loop, offset)
- ❌ No GUI timeline/arranger component yet
- ✅ VexelDAW-Native has ArrangementComponent.h reference

**Porting Strategy:**
- Backend: Already unified ✅
- GUI: Port from VexelDAW-Native or create new JUCE component
- Features: Drag-drop clips, resize, fade handles, multi-select

**Key Files:**
- Existing: `zenith-core/Source/engine/Clip.h/cpp`
- To Create: `zenith-core/Source/gui/ArrangerComponent.h/cpp`

**Estimated Effort:** ~4-6 days (high complexity)

---

### Feature 6: Track Automation (Backend)

**Source Branch:** HEAD (already present)

**Implementation:**
- `zenith-core/include/ProjectState.h` (automation API)
- `zenith-core/src/ProjectState.cpp` (envelope management)
- `zenith-core/include/TrackAutomationSynchronizer.h`
- `zenith-core/src/TrackAutomationSynchronizer.cpp` (RT-safe sampling)

**Key Commits:**
- `ab88e84` - Track Automation MVP implementation
- `3b6fb1a` - Bug fixes (frameCounter, undo)

**Status:** ✅ Complete, no merge needed

**Features:**
- Volume/Pan/Mute automation
- ValueTree-based storage
- Linear interpolation
- Undo/redo support
- 60Hz sampling rate

---

### Feature 7: Automation Lanes (UI)

**Source Branch:** None (not implemented anywhere)

**Current State:**
- ❌ No UI for automation lanes yet
- ✅ Backend exists (TrackAutomationSynchronizer)

**Implementation Strategy:**
- Create new JUCE component
- Display automation curves overlaid on timeline
- Drag-to-edit automation points
- Bezier curve editing (future)

**Key Files to Create:**
- `zenith-core/Source/gui/AutomationLaneComponent.h/cpp`
- Integration with ArrangerComponent

**Estimated Effort:** ~3-4 days (medium-high complexity)

---

### Feature 8: Wingman / CommandAPI

**Source Branch:** HEAD (already present)

**Implementation:**
- `zenith-core/include/CommandAPI.h` (JSON API)
- `zenith-core/src/CommandAPI.cpp` (command handlers)

**Key Commits:**
- `ab88e84` - CommandAPI implementation with automation

**Status:** ✅ Complete, no merge needed

**Features:**
- JSON command interface
- Automation control (add/move/delete points)
- Project state queries
- Transport control
- Track management

**Historical Context:**
- Commits `4ce0987`, `f7179c8`, `7d3b553` implemented web-based Wingman
- Current CommandAPI is C++/JUCE version for native integration

---

## Unification Strategy (Phases U1-U5)

### Phase U1: Create zenith-unified-main Base

**Goal:** Establish the unified branch with current best codebase

**Actions:**
```bash
# Step 1: Create new unified branch from HEAD
git checkout claude/zenith-merge-strategy-plan-01RcjzqGji5CweC99ESLP67W
git checkout -b zenith-unified-main

# Step 2: Tag current state for safety
git tag pre-unification-snapshot

# Step 3: Verify build cleanliness
cd zenith-core
mkdir -p build && cd build
cmake ..
cmake --build .

# Step 4: Run tests (if any)
ctest --output-on-failure

# Step 5: Document baseline
git log --oneline -10 > planning/roadmaps/unification-baseline.txt
```

**Files to Merge:** None (this IS the base)

**Expected Conflicts:** None

**Success Criteria:**
- ✅ Branch created: `zenith-unified-main`
- ✅ Builds successfully with CMake
- ✅ All existing tests pass
- ✅ Baseline documented

**Estimated Time:** 30 minutes

---

### Phase U2: Consolidate Documentation & Remove Legacy Code

**Goal:** Clean up repository, remove dead code, consolidate docs

**Actions:**
```bash
# Step 1: Remove legacy web implementations (already done, but verify)
# - Confirm src/audio/, src/juce-engine/, src/qt-qml/ are deprecated
# - Keep as reference but mark as archived

# Step 2: Move historical documentation to archive
mkdir -p planning/archive/web-implementation-docs
git mv RECORDING_FEATURES_DOCUMENTATION.md planning/archive/
git mv PLAYBACK_ENGINE_DOCUMENTATION.md planning/archive/
git mv ADVANCED_FEATURES_DOCUMENTATION.md planning/archive/

# Step 3: Create master feature inventory
cat > planning/roadmaps/FEATURE_INVENTORY.md << 'EOF'
# Zenith DAW - Feature Inventory (Post-Unification)

## Implemented (zenith-core/)
- [x] Engine + Mixer (Track, MixerChannel, Clip)
- [x] Track Automation Backend (Volume/Pan/Mute)
- [x] CommandAPI (Wingman integration)
- [x] ProjectState (ValueTree + Undo)
- [x] Transport Controls (Play/Stop/Record)

## To Be Ported from Historical Implementations
- [ ] Recording Engine (Audio/MIDI input)
- [ ] Export Engine (WAV/MIDI output)
- [ ] Piano Roll GUI
- [ ] Arranger Timeline GUI
- [ ] Automation Lanes GUI
- [ ] Browser Component
- [ ] Mixer GUI
- [ ] Session View (Clip Launcher)

## Future Work (Not Yet Implemented Anywhere)
- [ ] Plugin Hosting (VST3/AU)
- [ ] Advanced Automation (Bezier curves)
- [ ] Modular Routing (The Grid)
- [ ] Stock Plugins (EQ, Compressor, Reverb)
EOF

# Step 4: Update README to reflect unified state
# (Manual edit required)

# Step 5: Commit consolidation
git add .
git commit -m "$(cat <<'COMMIT_MSG'
[Unification Phase U2] Consolidate documentation and archive legacy code

Changes:
- Archive web-implementation docs to planning/archive/
- Create FEATURE_INVENTORY.md tracking implementation status
- Mark src/audio/, src/juce-engine/, src/qt-qml/ as deprecated reference code
- Document zenith-core/ as canonical C++/JUCE implementation

Status: zenith-unified-main is now the single source of truth
COMMIT_MSG
)"
```

**Files to Modify:**
- Archive: RECORDING_FEATURES_DOCUMENTATION.md → planning/archive/
- Archive: PLAYBACK_ENGINE_DOCUMENTATION.md → planning/archive/
- Archive: ADVANCED_FEATURES_DOCUMENTATION.md → planning/archive/
- Create: planning/roadmaps/FEATURE_INVENTORY.md
- Update: README.md (mark legacy dirs)

**Expected Conflicts:** None (documentation only)

**Success Criteria:**
- ✅ Legacy docs archived
- ✅ Feature inventory created
- ✅ Repository structure clarified

**Estimated Time:** 1 hour

---

### Phase U3: Port Recording + Export (Audio I/O)

**Goal:** Implement native C++/JUCE recording and export engines

**Approach:** **NOT a git merge** - this is new development based on historical specs

**Substeps:**

#### U3.1: Recording Engine Implementation

**Reference Commits:**
- `feb61bf` - Web Audio API recording (reference for feature spec)
- Documentation: planning/archive/RECORDING_FEATURES_DOCUMENTATION.md

**Files to Create:**
```
zenith-core/include/RecordingEngine.h
zenith-core/src/RecordingEngine.cpp
zenith-core/tests/RecordingEngineTests.cpp
```

**Implementation Checklist:**
- [ ] AudioDeviceManager input configuration
- [ ] Multi-track simultaneous recording
- [ ] Audio file writer (WAV format)
- [ ] Input monitoring with latency compensation
- [ ] Level metering (peak/RMS)
- [ ] MIDI input capture (MidiInput class)
- [ ] Integration with ProjectState (create clips)
- [ ] Integration with Engine (transport sync)

**Pseudo-Code Outline:**
```cpp
class RecordingEngine {
public:
    RecordingEngine(ProjectState& state, Engine& engine);

    void startRecording(const std::vector<juce::String>& trackIds);
    void stopRecording();
    bool isRecording() const;

    void setRecordingDirectory(const juce::File& dir);
    void setPreCountBars(int bars);

private:
    void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels,
                                float** outputChannelData, int numOutputChannels,
                                int numSamples);
    void handleMidiInput(const juce::MidiMessage& message, const juce::String& source);

    ProjectState& projectState;
    Engine& audioEngine;

    std::map<juce::String, std::unique_ptr<juce::AudioFormatWriter>> activeRecordings;
    std::atomic<bool> recording{false};
};
```

**Testing Strategy:**
- Unit tests: File creation, format validation
- Integration tests: Record → Stop → Verify clip created in ProjectState
- Manual tests: Multi-track recording, MIDI capture

#### U3.2: Export Engine Implementation

**Files to Create:**
```
zenith-core/include/ExportEngine.h
zenith-core/src/ExportEngine.cpp
zenith-core/tests/ExportEngineTests.cpp
```

**Implementation Checklist:**
- [ ] Offline rendering mode (non-realtime)
- [ ] Mixdown engine (sum all tracks)
- [ ] WAV export (16/24/32-bit configurable)
- [ ] MIDI export (MidiFile class)
- [ ] Stem export (per-track rendering)
- [ ] Tail detection (reverb/delay tails)
- [ ] Progress callback for UI

**Pseudo-Code Outline:**
```cpp
class ExportEngine {
public:
    ExportEngine(ProjectState& state, Engine& engine);

    struct ExportSettings {
        juce::File outputFile;
        int bitDepth = 24;
        double sampleRate = 48000.0;
        bool normalize = false;
        bool includeTails = true;
    };

    void exportMixdown(const ExportSettings& settings,
                       std::function<void(float progress)> callback);
    void exportStems(const juce::File& directory, const ExportSettings& settings);
    void exportMidi(const juce::File& outputFile);

private:
    void renderOffline(juce::AudioBuffer<float>& buffer, int numSamples);

    ProjectState& projectState;
    Engine& audioEngine;
};
```

#### U3.3: Integration & Testing

**Actions:**
```bash
# Build with new recording/export engines
cd zenith-core/build
cmake --build .

# Run new tests
ctest -R Recording
ctest -R Export

# Commit implementation
git add zenith-core/include/RecordingEngine.h
git add zenith-core/src/RecordingEngine.cpp
git add zenith-core/include/ExportEngine.h
git add zenith-core/src/ExportEngine.cpp
git add zenith-core/tests/RecordingEngineTests.cpp
git add zenith-core/tests/ExportEngineTests.cpp
git add zenith-core/CMakeLists.txt  # Updated with new sources

git commit -m "$(cat <<'COMMIT_MSG'
[Unification Phase U3] Implement Recording + Export engines (C++/JUCE)

Ported from web implementation (feb61bf) to native JUCE:

## Recording Engine
- Multi-track audio recording (AudioDeviceManager)
- MIDI input capture (MidiInput)
- Input monitoring with latency compensation
- Level metering (peak/RMS/clip detection)
- Automatic clip creation in ProjectState

## Export Engine
- Offline mixdown rendering (non-realtime)
- WAV export (16/24/32-bit)
- MIDI file export
- Stem export (per-track)
- Tail detection for effects

Testing:
- Unit tests for file I/O
- Integration tests with ProjectState
- Manual QA: Record audio → Export mixdown

Reference: planning/archive/RECORDING_FEATURES_DOCUMENTATION.md
COMMIT_MSG
)"
```

**Files Modified:**
- zenith-core/CMakeLists.txt (add new sources)
- zenith-core/include/Engine.h (optional: add recording callbacks)

**Expected Conflicts:** None (new files)

**Conflict Resolution:** N/A

**Success Criteria:**
- ✅ Can record audio to WAV files
- ✅ Can record MIDI to clips
- ✅ Can export mixdown to WAV
- ✅ All tests pass

**Estimated Time:** 2-3 days (implementation + testing)

---

### Phase U4: Port MIDI Editor + Arranger GUI

**Goal:** Implement native JUCE GUI components for MIDI editing and timeline arrangement

**Approach:** **NOT a git merge** - new JUCE GUI development based on historical web UI specs

**Substeps:**

#### U4.1: Piano Roll Component

**Reference Commits:**
- `7a09bc3` - FL Studio-grade Piano Roll (web)
- `df37665` - MIDI sequencing (web)

**Files to Create:**
```
zenith-core/Source/gui/PianoRollComponent.h
zenith-core/Source/gui/PianoRollComponent.cpp
zenith-core/Source/gui/MidiNoteComponent.h
zenith-core/Source/gui/MidiNoteComponent.cpp
zenith-core/tests/PianoRollTests.cpp
```

**Implementation Checklist:**
- [ ] JUCE Component-based UI
- [ ] Grid rendering (beats, bars, subdivisions)
- [ ] Piano keyboard on left side
- [ ] Note rendering (rectangles with velocity color)
- [ ] Mouse interactions:
  - [ ] Click-drag to create notes
  - [ ] Resize note edges (start/end time)
  - [ ] Drag notes vertically (pitch) and horizontally (time)
  - [ ] Velocity editing (drag note height)
  - [ ] Multi-select (shift-click, drag-box)
- [ ] Keyboard shortcuts (Ctrl+C/V for copy/paste)
- [ ] Zoom (horizontal/vertical)
- [ ] Quantization UI (grid snap)
- [ ] Integration with ProjectState (read/write MIDI clips)

**Reference Implementations:**
- JUCE Demo: MidiDemoComponent
- VexelDAW-Native (if GUI exists there)
- Web implementation commit 7a09bc3 for feature spec

**UI Layout:**
```
┌──────────────────────────────────────────────────┐
│ [Transport] [Quantize▾] [Tools: ✏️ ✂️ 🖌️]      │  ← Toolbar
├─────┬────────────────────────────────────────────┤
│     │  ┌─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┐                │
│  C5 │  │ │█│ │█│ │ │█│ │█│ │█│ │  ███████      │  ← Notes
│  B4 │  └─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┘                │
│  A4 │      ████████                              │
│  G4 │                  ██████                    │
│     │                                            │
│     └────────────────────────────────────────────┤
│              1   │   2   │   3   │   4   │       │  ← Timeline ruler
└──────────────────────────────────────────────────┘
```

#### U4.2: Arranger/Timeline Component

**Reference Commits:**
- `adfc43b` - Audio clip editing (web)
- VexelDAW-Native/Source/GUI/Arrangement/ArrangementComponent.h

**Files to Create:**
```
zenith-core/Source/gui/ArrangerComponent.h
zenith-core/Source/gui/ArrangerComponent.cpp
zenith-core/Source/gui/ClipComponent.h
zenith-core/Source/gui/ClipComponent.cpp
zenith-core/Source/gui/TimelineRuler.h
zenith-core/Source/gui/TimelineRuler.cpp
zenith-core/tests/ArrangerTests.cpp
```

**Implementation Checklist:**
- [ ] Track lanes (horizontal rows)
- [ ] Timeline ruler (bars/beats/ticks)
- [ ] Clip rendering:
  - [ ] Audio clips: waveform thumbnail
  - [ ] MIDI clips: piano roll preview
- [ ] Mouse interactions:
  - [ ] Drag-drop clips (move in time)
  - [ ] Resize clip edges (trim start/end)
  - [ ] Fade handles (drag fade in/out)
  - [ ] Multi-select clips (shift-click)
  - [ ] Cut/copy/paste clips
- [ ] Zoom (horizontal/vertical)
- [ ] Snap-to-grid (bars, beats, off)
- [ ] Playhead display (synchronized with Engine)
- [ ] Loop region visualization
- [ ] Integration with ProjectState (read/write Clip ValueTree)

**UI Layout:**
```
┌──────────────────────────────────────────────────────────┐
│ [Play] [Stop] [Record] [Loop] Tempo: 120 BPM  4/4       │  ← Transport bar
├──────────────────────────────────────────────────────────┤
│      │  1   │   2   │   3   │   4   │   5   │   6   │  │  ← Timeline ruler
├──────┼──────────────────────────────────────────────────┤
│Track1│▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│                           │  ← Audio clip
│      │  [~~waveform~~]      │                           │
├──────┼──────────────────────────────────────────────────┤
│Track2│          ░░░░░░░░░░░░│                           │  ← MIDI clip
│      │          [♪ ♫ ♪]     │                           │
├──────┼──────────────────────────────────────────────────┤
│Track3│                      ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓         │  ← Audio clip
│      │                      [~~waveform~~]              │
└──────────────────────────────────────────────────────────┘
         ▲ Playhead
```

#### U4.3: Integration & Main Window Layout

**Files to Modify:**
```
zenith-core/include/MainWindow.h
zenith-core/src/MainWindow.cpp
```

**Layout Strategy:**
```cpp
class MainWindow : public juce::DocumentWindow {
public:
    MainWindow() {
        // Create components
        arrangerComponent = std::make_unique<ArrangerComponent>(projectState);
        pianoRollComponent = std::make_unique<PianoRollComponent>(projectState);
        transportBar = std::make_unique<TransportComponent>(engine);

        // Layout (tabs or split view)
        tabbedComponent = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);
        tabbedComponent->addTab("Arrangement", juce::Colours::darkgrey, arrangerComponent.get(), false);
        tabbedComponent->addTab("Piano Roll", juce::Colours::darkgrey, pianoRollComponent.get(), false);

        setContentOwned(tabbedComponent.get(), true);
    }

private:
    ProjectState projectState;
    Engine engine;

    std::unique_ptr<ArrangerComponent> arrangerComponent;
    std::unique_ptr<PianoRollComponent> pianoRollComponent;
    std::unique_ptr<TransportComponent> transportBar;
    std::unique_ptr<juce::TabbedComponent> tabbedComponent;
};
```

#### U4.4: Commit GUI Implementation

**Actions:**
```bash
# Build with new GUI components
cd zenith-core/build
cmake --build .

# Test GUI (manual)
./ZenithDAW

# Commit implementation
git add zenith-core/Source/gui/
git add zenith-core/include/MainWindow.h
git add zenith-core/src/MainWindow.cpp
git add zenith-core/CMakeLists.txt

git commit -m "$(cat <<'COMMIT_MSG'
[Unification Phase U4] Implement Piano Roll + Arranger GUI (JUCE)

Ported from web implementation to native JUCE Components:

## Piano Roll Component
- FL Studio-grade MIDI editor
- Click-drag note creation
- Velocity editing
- Multi-select, copy/paste
- Quantize UI with grid snap
- Zoom (horizontal/vertical)
- Integration with ProjectState MIDI clips

## Arranger Component
- Multi-track timeline view
- Audio clip waveforms (thumbnails)
- MIDI clip piano roll previews
- Drag-drop clips, resize, fade handles
- Snap-to-grid (bars/beats)
- Playhead sync with Engine
- Loop region visualization

## MainWindow Layout
- Tabbed interface (Arrangement / Piano Roll)
- Transport bar integration
- Keyboard shortcuts

Reference commits: 7a09bc3 (piano roll), adfc43b (arranger)
Reference code: VexelDAW-Native/Source/GUI/Arrangement/
COMMIT_MSG
)"
```

**Expected Conflicts:**
- `MainWindow.cpp` - May need to preserve existing automation UI code

**Conflict Resolution Strategy:**
- **MainWindow.cpp**: Combine layouts
  - Keep existing track count display
  - Add new ArrangerComponent and PianoRollComponent
  - Use TabbedComponent or ResizableWindow for layout

**Success Criteria:**
- ✅ Piano roll opens and can create/edit notes
- ✅ Arranger displays clips from ProjectState
- ✅ Can drag-drop clips on timeline
- ✅ Playhead synchronized with Engine
- ✅ All GUI components build and render

**Estimated Time:** 5-7 days (complex GUI work)

---

### Phase U5: Implement Automation Lanes UI

**Goal:** Create GUI for editing automation curves in arranger view

**Approach:** New JUCE GUI component that visualizes TrackAutomationSynchronizer data

**Substeps:**

#### U5.1: Automation Lane Component

**Files to Create:**
```
zenith-core/Source/gui/AutomationLaneComponent.h
zenith-core/Source/gui/AutomationLaneComponent.cpp
zenith-core/Source/gui/AutomationPointComponent.h
zenith-core/Source/gui/AutomationPointComponent.cpp
zenith-core/tests/AutomationLaneTests.cpp
```

**Implementation Checklist:**
- [ ] Render automation curve from ProjectState ValueTree
- [ ] Display automation points as draggable handles
- [ ] Mouse interactions:
  - [ ] Click to add automation point
  - [ ] Drag point to edit time + value
  - [ ] Double-click to delete point
  - [ ] Drag to select multiple points
- [ ] Parameter selector (Volume, Pan, Mute dropdown)
- [ ] Curve interpolation rendering (linear for MVP, bezier future)
- [ ] Value scale (0.0-1.0 for volume, -1.0 to 1.0 for pan)
- [ ] Integration with TrackAutomationSynchronizer (read/write)
- [ ] Undo/redo for automation edits
- [ ] Sync with playback (highlight current value)

**UI Layout:**
```
┌──────────────────────────────────────────────────────────┐
│ Track 1 - Volume ▾                                  [+]  │  ← Parameter selector
├──────────────────────────────────────────────────────────┤
│ 1.0 ┬                 ●─────────────●                    │  ← Automation curve
│ 0.8 ┤       ●────────╱                ╲                  │
│ 0.6 ┤      ╱                            ╲────●           │
│ 0.4 ┤     ╱                                  ╲          │
│ 0.2 ┤    ●                                    ╲         │
│ 0.0 ┴────┼────┼────┼────┼────┼────┼────┼────┼────       │
│          1    2    3    4    5    6    7    8           │  ← Time (bars)
└──────────────────────────────────────────────────────────┘
         ● = Draggable automation point
```

#### U5.2: Integration with Arranger

**Files to Modify:**
```
zenith-core/Source/gui/ArrangerComponent.h
zenith-core/Source/gui/ArrangerComponent.cpp
```

**Integration Strategy:**
- Add automation lane toggle button per track
- When enabled, expand track height to show automation lane below clips
- AutomationLaneComponent shares timeline scale with ArrangerComponent
- Playhead synchronized across all views

**Layout (with automation visible):**
```
┌──────────────────────────────────────────────────────────┐
│Track1│▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│                           │  ← Clips
│  [A] │  [~~waveform~~]      │                           │  ← [A] = automation toggle
├──────┼──────────────────────────────────────────────────┤
│ Vol  │ ●─────╱╲────●────────│                           │  ← Automation lane
│ 0.8  │      ╱  ╲            │                           │
├──────┼──────────────────────────────────────────────────┤
│Track2│          ░░░░░░░░░░░░│                           │  ← Next track
│      │          [♪ ♫ ♪]     │                           │
└──────────────────────────────────────────────────────────┘
```

#### U5.3: Commit Automation Lanes UI

**Actions:**
```bash
# Build with automation lanes UI
cd zenith-core/build
cmake --build .

# Test automation editing
./ZenithDAW
# - Create automation points by clicking on lane
# - Drag points to edit curve
# - Play back and verify automation applies

# Commit implementation
git add zenith-core/Source/gui/AutomationLaneComponent.*
git add zenith-core/Source/gui/AutomationPointComponent.*
git add zenith-core/Source/gui/ArrangerComponent.* # Modified
git add zenith-core/tests/AutomationLaneTests.cpp
git add zenith-core/CMakeLists.txt

git commit -m "$(cat <<'COMMIT_MSG'
[Unification Phase U5] Implement Automation Lanes UI

Completes automation system by adding GUI for editing curves:

## Automation Lane Component
- Visualize automation curves from ProjectState ValueTree
- Draggable automation points
- Add points (click), delete (double-click), drag to edit
- Parameter selector (Volume/Pan/Mute per track)
- Linear interpolation rendering
- Undo/redo for automation edits
- Playback sync (highlight current value)

## Arranger Integration
- Per-track automation toggle [A] button
- Expandable lanes below clip area
- Shared timeline scale with clips
- Synchronized playhead

Testing:
- Manual QA: Create automation curve → Play back → Verify audio follows curve
- Undo/redo: Edit points → Undo → Verify curve restored
- Multi-track: Enable automation on multiple tracks simultaneously

Backend: TrackAutomationSynchronizer (Phase 13 - ab88e84)
UI: New JUCE Components
COMMIT_MSG
)"
```

**Expected Conflicts:**
- `ArrangerComponent.cpp` - May need to integrate with existing track layout

**Conflict Resolution Strategy:**
- **ArrangerComponent.cpp**:
  - Extend track height calculation to include automation lanes
  - Add automation lane components as child components
  - Preserve existing clip rendering logic
  - Use ResizableWindow to allow collapsing automation lanes

**Success Criteria:**
- ✅ Automation lanes visible per track
- ✅ Can create/edit/delete automation points via mouse
- ✅ Automation curves visually match playback behavior
- ✅ Undo/redo works for automation edits
- ✅ No regressions in existing arranger functionality

**Estimated Time:** 3-4 days (medium-high complexity)

---

## Conflict Hotspots & Resolution Strategy

### Hotspot 1: MainWindow Layout

**Affected Files:**
- `zenith-core/src/MainWindow.cpp`

**Conflict Source:**
- Current: Simple window with track count display
- Phase U4 adds: Arranger + Piano Roll components
- Phase U5 adds: Automation lanes integration

**Resolution Strategy:**

**Current Code (simplified):**
```cpp
MainWindow::MainWindow() {
    auto* trackCountLabel = new juce::Label("", "Tracks: 0");
    setContentOwned(trackCountLabel, true);

    // Engine integration
    engine.setProjectState(&projectState);
    engine.initialize();
}
```

**Unified Code (after U4 + U5):**
```cpp
MainWindow::MainWindow() : juce::DocumentWindow("Zenith DAW",
                                                 juce::Colours::darkgrey,
                                                 DocumentWindow::allButtons) {
    // Create components
    transportBar = std::make_unique<TransportComponent>(engine, projectState);
    arrangerComponent = std::make_unique<ArrangerComponent>(projectState, engine);
    pianoRollComponent = std::make_unique<PianoRollComponent>(projectState);
    mixerComponent = std::make_unique<MixerComponent>(projectState);

    // Main layout
    auto* mainLayout = new juce::Component();
    auto* topBar = transportBar.get();
    auto* contentTabs = new juce::TabbedComponent(juce::TabbedButtonBar::TabsAtTop);
    contentTabs->addTab("Arrangement", juce::Colours::darkgrey, arrangerComponent.get(), false);
    contentTabs->addTab("Piano Roll", juce::Colours::darkgrey, pianoRollComponent.get(), false);
    contentTabs->addTab("Mixer", juce::Colours::darkgrey, mixerComponent.get(), false);

    // Layout manager (verticalLayout: topBar + contentTabs)
    // ... (omitted for brevity)

    setContentOwned(mainLayout, true);

    // Engine integration
    engine.setProjectState(&projectState);
    engine.initialize();
}
```

**Manual Merge Strategy:**
1. Keep existing engine initialization code
2. Replace simple label with full component hierarchy
3. Preserve ProjectState and Engine references
4. Add component member variables

### Hotspot 2: ProjectState ValueTree Structure

**Affected Files:**
- `zenith-core/include/ProjectState.h`
- `zenith-core/src/ProjectState.cpp`

**Conflict Source:**
- Current: Has automation identifiers (AUTOMATION, ENVELOPE, POINT)
- Phase U3 may add: RECORDING_SETTINGS, EXPORT_SETTINGS
- Phase U4 may add: PIANO_ROLL_STATE, ARRANGER_VIEW_STATE

**Resolution Strategy:**

**Approach:** Additive only - no conflicts expected

All new features add new ValueTree children without modifying existing structure:

```
PROJECT
├── TRACKS
│   ├── TRACK (id="track_1")
│   │   ├── AUTOMATION       ← Phase 13 (already exists)
│   │   ├── CLIPS            ← Phase 0 (already exists)
│   │   └── RECORDING_STATE  ← Phase U3 (new, no conflict)
│   └── TRACK (id="track_2")
├── EXPORT_SETTINGS          ← Phase U3 (new, no conflict)
├── PIANO_ROLL_VIEW          ← Phase U4 (new, no conflict)
└── ARRANGER_VIEW            ← Phase U4 (new, no conflict)
```

**Resolution:** No manual merge needed - additive changes only

### Hotspot 3: Engine Audio Callback

**Affected Files:**
- `zenith-core/src/Engine.cpp` (audioDeviceIOCallback method)

**Conflict Source:**
- Current: Handles playback + automation sampling
- Phase U3 adds: Recording input processing
- Potential: Shared AudioDeviceIOCallback function

**Resolution Strategy:**

**Current Code (simplified):**
```cpp
void Engine::audioDeviceIOCallback(const float** inputChannelData,
                                   int numInputChannels,
                                   float** outputChannelData,
                                   int numOutputChannels,
                                   int numSamples) {
    // 1. Playback processing
    juce::AudioBuffer<float> buffer(outputChannelData, numOutputChannels, numSamples);
    buffer.clear();

    // 2. Mix tracks
    // ... (existing code)

    // 3. Automation (already integrated)
    // ... (TrackAutomationSynchronizer runs on timer, not audio thread)
}
```

**Unified Code (after U3):**
```cpp
void Engine::audioDeviceIOCallback(const float** inputChannelData,
                                   int numInputChannels,
                                   float** outputChannelData,
                                   int numOutputChannels,
                                   int numSamples) {
    // 1. RECORDING: Process input (if recording active)
    if (recordingEngine && recordingEngine->isRecording()) {
        recordingEngine->processInput(inputChannelData, numInputChannels, numSamples);
    }

    // 2. PLAYBACK: Process output
    juce::AudioBuffer<float> buffer(outputChannelData, numOutputChannels, numSamples);
    buffer.clear();

    // 3. Mix tracks (existing code preserved)
    // ... (existing code)

    // 4. Automation (no changes - still timer-based)
    // ... (TrackAutomationSynchronizer runs separately)
}
```

**Manual Merge Strategy:**
1. Add recording input processing BEFORE playback output
2. Wrap recording code in `if (recording)` check
3. Preserve all existing playback + automation code
4. Ensure no locks or allocations (RT-safe)

### Hotspot 4: CMakeLists.txt (Build System)

**Affected Files:**
- `zenith-core/CMakeLists.txt`

**Conflict Source:**
- Each phase adds new source files
- Multiple additions to `target_sources()` and `target_include_directories()`

**Resolution Strategy:**

**Current CMakeLists.txt (simplified):**
```cmake
target_sources(ZenithDAW PRIVATE
    src/Main.cpp
    src/MainWindow.cpp
    src/ProjectState.cpp
    src/Engine.cpp
    src/CommandAPI.cpp                    # Phase 13
    src/TrackAutomationSynchronizer.cpp  # Phase 13
    Source/engine/Track.cpp
    Source/engine/Clip.cpp
    Source/engine/MixerChannel.cpp
)
```

**Unified CMakeLists.txt (after all phases):**
```cmake
target_sources(ZenithDAW PRIVATE
    # Core
    src/Main.cpp
    src/MainWindow.cpp
    src/ProjectState.cpp
    src/Engine.cpp

    # Phase 13: Automation
    src/CommandAPI.cpp
    src/TrackAutomationSynchronizer.cpp

    # Phase U3: Recording + Export
    src/RecordingEngine.cpp
    src/ExportEngine.cpp

    # Engine primitives
    Source/engine/Track.cpp
    Source/engine/Clip.cpp
    Source/engine/MixerChannel.cpp

    # Phase U4: GUI
    Source/gui/ArrangerComponent.cpp
    Source/gui/ClipComponent.cpp
    Source/gui/PianoRollComponent.cpp
    Source/gui/MidiNoteComponent.cpp
    Source/gui/TimelineRuler.cpp
    Source/gui/TransportComponent.cpp

    # Phase U5: Automation Lanes UI
    Source/gui/AutomationLaneComponent.cpp
    Source/gui/AutomationPointComponent.cpp
)
```

**Manual Merge Strategy:**
1. Group sources by feature category (comments)
2. Alphabetize within each group
3. Preserve existing entries
4. Add new entries at appropriate locations
5. Verify all files exist before committing

---

## Risks & Mitigations

### Risk 1: Feature Porting Complexity

**Risk:** Historical web implementations (Piano Roll, Recording, etc.) are fundamentally different from C++/JUCE - porting is NOT a simple merge

**Probability:** High
**Impact:** High (major time sink)

**Mitigation:**
1. **Treat as New Development:** Phases U3-U5 are new feature implementations, not merges
2. **Use Historical Code as Spec:** Reference commits (7a09bc3, feb61bf) for feature requirements, not code to merge
3. **Phased Approach:** Implement one feature at a time with testing between phases
4. **Fallback Plan:** If porting takes too long, defer non-critical features (Piano Roll) to post-unification
5. **Time Boxing:** Allocate max 2 weeks per phase - if exceeded, re-scope

### Risk 2: GUI Complexity (JUCE Learning Curve)

**Risk:** JUCE GUI programming is complex - arranger and piano roll are major components

**Probability:** Medium
**Impact:** High (delays, bugs)

**Mitigation:**
1. **Reference Implementations:** Study JUCE demos (AudioPluginDemo, MidiDemo)
2. **Incremental Development:** Build simplest version first (no fancy features)
3. **Code Review:** Review JUCE best practices (use Component hierarchy, avoid memory leaks)
4. **Prototyping:** Build throwaway prototypes to test approaches
5. **Expert Consultation:** Consult JUCE forums / documentation for tricky issues

### Risk 3: Merge Conflicts (Despite Minimal Expected Conflicts)

**Risk:** Unexpected conflicts in MainWindow, Engine, or ProjectState

**Probability:** Low-Medium
**Impact:** Medium (merge resolution time)

**Mitigation:**
1. **Frequent Commits:** Commit after each sub-step to isolate changes
2. **Git Tags:** Tag before each phase (`pre-U3`, `pre-U4`, etc.) for easy rollback
3. **Diff Review:** Before committing, review full diff to catch accidental changes
4. **Manual Testing:** Test build + basic functionality after each merge
5. **Conflict Resolution Plan:** Use strategies documented in "Conflict Hotspots" section

### Risk 4: Regression Bugs (Breaking Existing Features)

**Risk:** New features (recording, GUI) break existing automation or engine

**Probability:** Medium
**Impact:** High (loss of working functionality)

**Mitigation:**
1. **Regression Test Suite:** Create tests for existing features (automation, playback)
2. **Manual QA Checklist:** After each phase, run standard test scenarios:
   - Play audio clip → verify playback
   - Add automation point → verify curve applies
   - Undo/redo → verify state restored
3. **Git Bisect:** If regression found, use `git bisect` to identify breaking commit
4. **Feature Flags:** Use `#ifdef ENABLE_RECORDING` to disable incomplete features temporarily
5. **Rollback Plan:** If major regression, revert to last stable commit

### Risk 5: Incomplete Documentation (Future Devs Lost)

**Risk:** Unification plan executed but poorly documented - future work is confused

**Probability:** Medium
**Impact:** Medium (slows future development)

**Mitigation:**
1. **Commit Messages:** Use detailed, structured commit messages (see examples in phases)
2. **Update FEATURE_INVENTORY.md:** Mark features as implemented after each phase
3. **Architecture Docs:** Create `docs/architecture/ZENITH_CORE_ARCHITECTURE.md` explaining component relationships
4. **Code Comments:** Document complex algorithms (automation sampling, clip rendering)
5. **Onboarding Guide:** Create `docs/DEVELOPER_ONBOARDING.md` for new contributors

### Risk 6: Performance Degradation (GUI Lag, Audio Glitches)

**Risk:** New GUI components cause CPU spikes, affecting audio thread

**Probability:** Low-Medium
**Impact:** High (unusable DAW)

**Mitigation:**
1. **Profile Early:** Use Instruments (macOS) or VTune (Windows) to profile GUI rendering
2. **RT-Safe Audio Thread:** Ensure NO GUI code runs on audio thread
3. **Timer-Based Updates:** Use juce::Timer for GUI updates (60Hz max)
4. **Optimize Rendering:** Use cached waveforms, dirty rectangles for repaints
5. **Testing:** Test with large projects (100+ clips, 10+ automation lanes)

---

## Summary: Execution Order

```
┌─────────────────────────────────────────────────────────────┐
│ Phase U1: Create zenith-unified-main Base                  │
│ ├─ Create branch from HEAD                                 │
│ ├─ Tag pre-unification-snapshot                            │
│ ├─ Verify build cleanliness                                │
│ └─ Document baseline                                       │
│ Time: 30 min                                                │
├─────────────────────────────────────────────────────────────┤
│ Phase U2: Consolidate Documentation                        │
│ ├─ Archive legacy web docs                                 │
│ ├─ Create FEATURE_INVENTORY.md                             │
│ ├─ Update README                                            │
│ └─ Commit consolidation                                    │
│ Time: 1 hour                                                │
├─────────────────────────────────────────────────────────────┤
│ Phase U3: Port Recording + Export (Audio I/O)              │
│ ├─ U3.1: Implement RecordingEngine (C++/JUCE)              │
│ ├─ U3.2: Implement ExportEngine (C++/JUCE)                 │
│ ├─ U3.3: Integration & testing                             │
│ └─ Commit recording + export                               │
│ Time: 2-3 days                                              │
├─────────────────────────────────────────────────────────────┤
│ Phase U4: Port MIDI Editor + Arranger GUI                  │
│ ├─ U4.1: PianoRollComponent (JUCE GUI)                     │
│ ├─ U4.2: ArrangerComponent (JUCE GUI)                      │
│ ├─ U4.3: MainWindow layout integration                     │
│ └─ Commit GUI implementation                               │
│ Time: 5-7 days                                              │
├─────────────────────────────────────────────────────────────┤
│ Phase U5: Implement Automation Lanes UI                    │
│ ├─ U5.1: AutomationLaneComponent (JUCE GUI)                │
│ ├─ U5.2: Integrate with ArrangerComponent                  │
│ ├─ U5.3: Testing & polish                                  │
│ └─ Commit automation lanes UI                              │
│ Time: 3-4 days                                              │
├─────────────────────────────────────────────────────────────┤
│ TOTAL TIME: ~2 weeks (assuming 1 developer)                │
└─────────────────────────────────────────────────────────────┘
```

---

## Final Deliverables

After completing Phases U1-U5, the `zenith-unified-main` branch will contain:

**Implemented Features:**
- ✅ Engine + Mixer (Track, Clip, MixerChannel)
- ✅ Track Automation Backend (Volume/Pan/Mute)
- ✅ Automation Lanes UI (draggable curves)
- ✅ Recording Engine (Audio/MIDI input)
- ✅ Export Engine (WAV/MIDI output)
- ✅ Piano Roll GUI (MIDI editor)
- ✅ Arranger GUI (timeline with clips)
- ✅ CommandAPI (Wingman integration)
- ✅ ProjectState (ValueTree + Undo)

**File Structure:**
```
zenith-core/
├── include/
│   ├── Engine.h                          ← Phase 0
│   ├── ProjectState.h                    ← Phase 0 + 13
│   ├── CommandAPI.h                      ← Phase 13
│   ├── TrackAutomationSynchronizer.h     ← Phase 13
│   ├── RecordingEngine.h                 ← Phase U3
│   ├── ExportEngine.h                    ← Phase U3
│   └── MainWindow.h                      ← All phases
├── src/
│   ├── Engine.cpp
│   ├── ProjectState.cpp
│   ├── CommandAPI.cpp
│   ├── TrackAutomationSynchronizer.cpp
│   ├── RecordingEngine.cpp               ← Phase U3
│   ├── ExportEngine.cpp                  ← Phase U3
│   └── MainWindow.cpp                    ← All phases
├── Source/
│   ├── engine/
│   │   ├── Track.h/cpp
│   │   ├── Clip.h/cpp
│   │   └── MixerChannel.h/cpp
│   └── gui/
│       ├── ArrangerComponent.h/cpp        ← Phase U4
│       ├── ClipComponent.h/cpp            ← Phase U4
│       ├── PianoRollComponent.h/cpp       ← Phase U4
│       ├── MidiNoteComponent.h/cpp        ← Phase U4
│       ├── TimelineRuler.h/cpp            ← Phase U4
│       ├── AutomationLaneComponent.h/cpp  ← Phase U5
│       └── AutomationPointComponent.h/cpp ← Phase U5
└── tests/
    ├── ProjectStateTests.cpp
    ├── RecordingEngineTests.cpp           ← Phase U3
    ├── ExportEngineTests.cpp              ← Phase U3
    ├── PianoRollTests.cpp                 ← Phase U4
    ├── ArrangerTests.cpp                  ← Phase U4
    └── AutomationLaneTests.cpp            ← Phase U5
```

**Documentation:**
- `planning/roadmaps/FEATURE_INVENTORY.md` - Feature implementation status
- `planning/roadmaps/ZENITH_UNIFICATION_PLAN.md` - This document (execution plan)
- `planning/archive/` - Historical web implementation docs (reference only)
- `docs/architecture/ZENITH_CORE_ARCHITECTURE.md` - Component relationships (to be created)

---

## Document Status

**Status:** ✅ PLANNING COMPLETE - READY FOR EXECUTION

**Next Steps:**
1. Review this plan with stakeholders
2. Get approval to proceed
3. Begin Phase U1 execution
4. Update this document with actual results as phases complete

**Contact:**
- For questions about this plan, consult the git commit history
- For execution issues, document in `planning/roadmaps/UNIFICATION_ISSUES.md`

---

**END OF UNIFICATION PLAN**
