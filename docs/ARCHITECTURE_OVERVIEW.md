# Zenith DAW - Architecture Overview

**Last Updated:** 2025-11-17
**Version:** 0.1.0
**Status:** Active Development

## Table of Contents

1. [Introduction](#introduction)
2. [High-Level Architecture](#high-level-architecture)
3. [Core Modules](#core-modules)
4. [Data Flow](#data-flow)
5. [Threading Model](#threading-model)
6. [File Structure](#file-structure)
7. [Technology Stack](#technology-stack)

---

## Introduction

Zenith DAW (formerly Vexel DAW) is a professional Digital Audio Workstation built with **pure C++ and JUCE 8**. The project has evolved from hybrid architectures (Electron, Qt/QML) to a fully native JUCE implementation focused on:

- **Native Performance:** 100% C++ JUCE 8 components, no web/Qt/Electron overhead
- **Real-Time Safety:** Lock-free audio processing with strict thread safety
- **Modern Architecture:** ValueTree-based state management with undo/redo
- **Windows-First:** Initial focus on Windows with MMCSS audio thread priority

### Design Philosophy

- **Single Source of Truth:** ProjectState (ValueTree) is the authoritative data source
- **Lock-Free Audio:** No allocations, locks, or system calls on audio thread
- **Modular Design:** Clear separation between Engine, UI, and State
- **Undo-Aware:** All mutations go through UndoManager

---

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Zenith DAW Application                       │
│                        (JUCE 8 / C++20)                              │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                    ┌──────────────┴──────────────┐
                    │                             │
          ┌─────────▼─────────┐       ┌──────────▼──────────┐
          │   MainWindow      │       │   Engine            │
          │   (UI Container)   │       │   (Audio Engine)    │
          └─────────┬─────────┘       └──────────┬──────────┘
                    │                             │
          ┌─────────▼─────────┐       ┌──────────▼──────────┐
          │   MainComponent   │       │  AudioDeviceManager │
          │   (Transport UI)   │       │  (JUCE Audio I/O)   │
          └─────────┬─────────┘       └──────────┬──────────┘
                    │                             │
                    │                 ┌───────────▼───────────┐
                    │                 │   Audio Callback      │
                    │                 │   (RT-Safe Thread)    │
                    │                 └───────────┬───────────┘
                    │                             │
          ┌─────────▼─────────┐       ┌──────────▼──────────┐
          │   ProjectState    │       │   Track (atomics)   │
          │   (ValueTree)     │◄──────┤   - volume          │
          └─────────┬─────────┘       │   - pan             │
                    │                 │   - mute            │
                    │                 └─────────────────────┘
          ┌─────────▼─────────────────┐
          │ TrackAutomationSynchronizer│
          │ (Message Thread, 60 Hz)    │
          │ - Samples automation curves │
          │ - Updates Track atomics     │
          └─────────────────────────────┘
```

### Component Interaction Flow

```
User Interaction → MainComponent → ProjectState (ValueTree) → UndoManager
                                         ↓
                         TrackAutomationSynchronizer (60 Hz Timer)
                                         ↓
                              Track atomics (volume, pan, mute)
                                         ↓
                         Engine::audioDeviceIOCallback() (Audio Thread)
                                         ↓
                              Track::getNextAudioBlock()
                                         ↓
                                  Audio Output
```

---

## Core Modules

### 1. Application Layer

**File:** `src/Main.cpp`
**Class:** `ZenithApplication`

**Responsibilities:**
- Application lifecycle (initialization, shutdown)
- System information logging
- Main window creation
- JUCE application framework integration

**Key Methods:**
- `initialise()`: Creates MainWindow and logs system info
- `shutdown()`: Clean resource cleanup
- `systemRequestedQuit()`: Handles OS quit requests

---

### 2. Main Window

**Files:** `include/MainWindow.h`, `src/MainWindow.cpp`
**Classes:** `MainWindow`, `MainComponent`

**MainWindow Responsibilities:**
- Top-level window management
- Engine and ProjectState ownership
- Menu bar (future)

**MainComponent Responsibilities:**
- Transport controls (Play, Stop, Record buttons)
- Status display (CPU usage, audio device info)
- Track count monitoring
- Timer-based UI updates (60 Hz)

**UI Components:**
- `playButton`, `stopButton`, `recordButton`: Transport controls
- `statusLabel`: Playback status
- `cpuLabel`: CPU usage percentage
- `audioDeviceLabel`: Audio device information
- `trackCountLabel`: Number of tracks

---

### 3. Audio Engine

**Files:** `include/Engine.h`, `src/Engine.cpp`
**Class:** `Engine`

**Responsibilities:**
- Audio device I/O management (via `AudioDeviceManager`)
- Transport state (play/stop/record)
- Audio processing callback (real-time safe)
- CPU usage monitoring
- Track container management
- Automation synchronization

**Key Properties:**
```cpp
std::atomic<bool> isPlaying_         // Transport state
std::atomic<bool> isRecording_       // Recording state
std::atomic<double> currentSampleRate // Audio settings
std::atomic<int> currentBufferSize   // Buffer size
std::atomic<int64> playbackPosition  // Sample-accurate position
```

**Audio Thread Methods:**
```cpp
void audioDeviceIOCallbackWithContext(...)  // Main audio callback
void processAudio(...)                      // Audio processing logic
```

**Threading Rules:**
- ✅ **Audio Thread:** Read atomics, process samples, no allocations
- ✅ **Message Thread:** Control flow, device management, UI updates

**Track Management:**
- `tracks_`: Container of `zenith::Track` instances
- `addTestTracks()`: Debug helper for creating demo tracks
- `getNumTracks()`: Thread-safe track count

---

### 4. Project State

**Files:** `include/ProjectState.h`, `src/ProjectState.cpp`
**Class:** `ProjectState`

**Responsibilities:**
- Single source of truth for all project data
- ValueTree-based state management
- Undo/redo support via `UndoManager`
- XML serialization (save/load projects)
- Track, clip, and automation data management

**ValueTree Schema:**
```
PROJECT
├── name: "Untitled"
├── tempo: 120.0
├── timeSignatureNumerator: 4
├── timeSignatureDenominator: 4
├── TRACKS
│   └── TRACK (id, name, type, volume, pan, mute, solo)
│       ├── CLIPS
│       │   └── CLIP (id, start, length, audioFile)
│       └── AUTOMATION
│           ├── ENVELOPE (param="volume")
│           │   └── POINT (id, timeBeats, value)
│           ├── ENVELOPE (param="pan")
│           └── ENVELOPE (param="mute")
└── MIXER
    └── masterVolume: 0.8
```

**Core API:**
```cpp
// Project management
void newProject()
bool loadFromFile(const File& file)
bool saveToFile(const File& file)

// Project properties
double getTempo() const
void setTempo(double tempo)

// Track management
String addTrack(const String& name, const String& type)
bool removeTrack(const String& trackId)

// Automation (Phase 13)
String addAutomationPoint(trackId, param, timeBeats, value, actionName)
bool moveAutomationPoint(trackId, param, pointId, newTime, newValue, actionName)
bool deleteAutomationPoint(trackId, param, pointId, actionName)
ValueTree getAutomationEnvelope(trackId, param) const
```

**Identifiers:**
- `ID_PROJECT`, `ID_TRACKS`, `ID_TRACK`, `ID_CLIPS`, `ID_CLIP`
- `ID_AUTOMATION`, `ID_ENVELOPE`, `ID_POINT`
- `PROP_NAME`, `PROP_TEMPO`, `PROP_VOLUME`, `PROP_PAN`, etc.

---

### 5. Track System

**Files:** `Source/engine/Track.h`, `Source/engine/Track.cpp`
**Namespace:** `zenith`
**Class:** `Track`

**Responsibilities:**
- Audio/MIDI track with clip playback
- Mixer controls (volume, pan, mute, solo, armed)
- Plugin chain (stubbed for Phase 2)
- Level metering

**Track Types:**
```cpp
enum class Type { Audio, MIDI, Instrument }
```

**Mixer Controls (Atomic):**
```cpp
std::atomic<float> volume{0.8f}    // 0.0 to 1.0
std::atomic<float> pan{0.0f}       // -1.0 (left) to 1.0 (right)
std::atomic<bool> muted{false}     // Mute state
std::atomic<bool> solo{false}      // Solo state
std::atomic<bool> armed{false}     // Recording armed
std::atomic<bool> enabled{true}    // Track enabled
```

**Audio Processing:**
```cpp
void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override
void processPluginChain(AudioBuffer<float>& buffer, int numSamples)
void applyGainAndPan(AudioBuffer<float>& buffer, int numSamples)
void updateLevelMeters(const AudioBuffer<float>& buffer, int numSamples)
```

**Clip Management:**
```cpp
void addClip(std::unique_ptr<Clip> clip)
void removeClip(int clipIndex)
int getNumClips() const
Clip* getClip(int index) const
```

---

### 6. Clip System

**Files:** `Source/engine/Clip.h`, `Source/engine/Clip.cpp`
**Namespace:** `zenith`
**Class:** `Track::Clip`

**Responsibilities:**
- Audio/MIDI clip playback
- Timeline positioning and length
- Fade in/out
- Looping
- Trimming (offset)

**Clip Types:**
```cpp
enum class Type { Audio, MIDI }
```

**Timeline Properties (Atomic):**
```cpp
std::atomic<int64_t> startPosition     // Start position in samples
std::atomic<int64_t> clipLength        // Length in samples
std::atomic<int64_t> clipOffset        // Offset for trimming
std::atomic<int64_t> transportPosition // Current playback position
std::atomic<bool> playing              // Playback state
```

**Audio Data:**
```cpp
File audioFile                         // Source audio file
AudioBuffer<float> audioBuffer         // In-memory audio data
AudioFormatReaderSource* audioSource   // Audio file reader
```

**MIDI Data:**
```cpp
MidiMessageSequence midiSequence       // MIDI events
```

**Fades:**
```cpp
std::atomic<int64_t> fadeInLength      // Fade in duration (samples)
std::atomic<int64_t> fadeOutLength     // Fade out duration (samples)
float calculateFadeMultiplier(int64_t positionInClip) const
```

---

### 7. Automation System

**Files:**
- `include/TrackAutomationSynchronizer.h`
- `src/TrackAutomationSynchronizer.cpp`

**Class:** `TrackAutomationSynchronizer`

**Responsibilities:**
- Bridge between ProjectState (ValueTree) and Engine (Track atomics)
- Timer-based automation curve sampling (60 Hz default)
- Beat-synchronized automation playback
- Linear interpolation between automation points

**How It Works:**
1. Listens to ProjectState automation envelopes (ValueTree)
2. Runs a timer on message thread (60 Hz)
3. Reads current playback position from Engine
4. Converts sample position → beats using tempo
5. Samples automation curves at current beat position
6. Interpolates between points (linear)
7. Writes sampled values to Track atomics

**Supported Parameters:**
- `"volume"`: 0.0 to 1.0
- `"pan"`: -1.0 (left) to 1.0 (right)
- `"mute"`: 0 (unmuted) or 1 (muted)

**RT-Safety:**
- ✅ No ValueTree access on audio thread
- ✅ No allocations on audio thread
- ✅ Only atomic writes (from message thread)
- ✅ Only atomic reads (from audio thread)

**Implementation:**
```cpp
void timerCallback() override
{
    // 1. Get current playback position (samples)
    auto position = engine.getPlaybackPosition();

    // 2. Convert to beats
    double beats = samplesToBeats(position);

    // 3. Sample each automation envelope
    for (auto& track : tracks)
    {
        auto envelope = projectState->getAutomationEnvelope(trackId, "volume");
        double value = sampleEnvelope(envelope, beats);
        track->setVolume(static_cast<float>(value));
    }
}
```

---

### 8. Command API

**Files:** `include/CommandAPI.h`, `src/CommandAPI.cpp`
**Class:** `CommandAPI`

**Responsibilities:**
- JSON command interface for Wingman AI integration
- Automation control
- Project manipulation
- Error handling and validation

**Command Format:**
```json
// Request
{
  "command": "add_automation_point",
  "params": {
    "trackId": "track_0",
    "param": "volume",
    "timeBeats": 8.0,
    "value": 0.5
  }
}

// Response (Success)
{
  "status": "ok",
  "data": {
    "pointId": "point_123"
  }
}

// Response (Error)
{
  "status": "error",
  "error": "Track not found: track_99"
}
```

**Built-in Commands:**
- `add_track`: Create new track
- `add_automation_point`: Add automation point
- `clear_automation`: Clear all automation for parameter
- `get_automation`: Get automation points
- `get_project_info`: Get project metadata
- `set_tempo`: Change project tempo

---

## Data Flow

### Recording Flow (Planned - Not Yet Implemented)

```
Audio Input → AudioDeviceManager
                    ↓
              Engine::audioDeviceIOCallback()
                    ↓
              RecordingEngine (TBD)
                    ↓
              AudioBuffer (captured samples)
                    ↓
              Clip (created from recording)
                    ↓
              ProjectState (ValueTree)
                    ↓
              Track (added to timeline)
```

### Editing Flow (Current)

```
User UI Action → MainComponent
                      ↓
                ProjectState::addAutomationPoint()
                      ↓
                ValueTree mutation + UndoManager
                      ↓
                ValueTree::Listener notification
                      ↓
       TrackAutomationSynchronizer::valueTreePropertyChanged()
                      ↓
           Timer samples envelope at next callback
                      ↓
                Track::setVolume() (atomic)
                      ↓
            Engine::audioDeviceIOCallback()
                      ↓
           Track::getNextAudioBlock()
                      ↓
           Track::applyGainAndPan() (reads atomic)
                      ↓
                 Audio Output
```

### Playback Flow

```
User presses Play → MainComponent::playButton clicked
                         ↓
                   Engine::play()
                         ↓
           isPlaying_ = true (atomic)
           TrackAutomationSynchronizer::startTimer()
                         ↓
        Engine::audioDeviceIOCallback() (Audio Thread)
                         ↓
                if (isPlaying_) → processAudio()
                         ↓
                for each Track:
                    Track::getNextAudioBlock()
                    Track::applyGainAndPan() (reads atomics)
                         ↓
                     Mix to output
                         ↓
                   Audio Hardware
```

### Save/Load Flow

```
Save:
  User → MainComponent → ProjectState::saveToFile()
                               ↓
                         ValueTree::toXmlString()
                               ↓
                         File::replaceWithText()
                               ↓
                         .zth file on disk

Load:
  User → MainComponent → ProjectState::loadFromFile()
                               ↓
                         File::loadFileAsString()
                               ↓
                         ValueTree::fromXml()
                               ↓
                         ValueTree listeners notified
                               ↓
                         UI updates automatically
```

---

## Threading Model

Zenith DAW uses **two primary threads** with strict separation:

### Audio Thread (Real-Time Priority)

**Entry Point:** `Engine::audioDeviceIOCallbackWithContext()`

**Responsibilities:**
- Process audio samples (read input, write output)
- Read Track atomics (volume, pan, mute)
- Update playback position counter
- Generate test tone (debug)

**NEVER ALLOWED:**
- ❌ Memory allocation (`new`, `malloc`, `std::vector::push_back`)
- ❌ Mutex locks (`std::mutex`, `std::lock_guard`, `CriticalSection`)
- ❌ System calls (file I/O, logging, network)
- ❌ UI updates (`repaint()`, `setBounds()`)
- ❌ ValueTree access (not thread-safe)

**ALWAYS ALLOWED:**
- ✅ Read `std::atomic` values
- ✅ Write to pre-allocated audio buffers
- ✅ Fixed-size math operations
- ✅ Lock-free data structures (SPSC queues)

**Priority:**
- Windows: MMCSS "Pro Audio" thread priority (if enabled)
- macOS: CoreAudio sets real-time priority automatically
- Linux: JACK/ALSA real-time scheduling

---

### Message Thread (Normal Priority)

**Responsibilities:**
- UI updates and event handling
- ValueTree mutations (ProjectState)
- Automation sampling (TrackAutomationSynchronizer)
- Audio device management
- File I/O (save/load projects)
- Undo/redo operations

**Communication with Audio Thread:**
- **Write:** `std::atomic` variables (volume, pan, mute)
- **Read:** `std::atomic` variables (playback position, CPU usage)
- **Never:** Direct function calls into audio thread

---

### Thread Communication Patterns

#### Pattern 1: Simple Parameters (std::atomic)

```cpp
// Message Thread
track->setVolume(0.8f);  // Writes to std::atomic<float> volume

// Audio Thread (audioDeviceIOCallback)
float vol = track->getVolume();  // Reads std::atomic<float> volume
buffer *= vol;
```

#### Pattern 2: ValueTree Listeners

```cpp
// ProjectState (Message Thread)
projectState.addAutomationPoint("track_0", "volume", 4.0, 0.5, "Add point");
    ↓
ValueTree mutation
    ↓
ValueTree::Listener::valueTreePropertyChanged() on TrackAutomationSynchronizer
    ↓
TrackAutomationSynchronizer::timerCallback() samples envelope
    ↓
Track::setVolume() writes to atomic
    ↓
Audio thread reads atomic in next callback
```

#### Pattern 3: Lock-Free FIFO (Future - MIDI Events)

```cpp
// Message Thread: Push MIDI event
midiQueue.push(midiMessage);

// Audio Thread: Pop MIDI event
if (midiQueue.pop(message))
    processMidiMessage(message);
```

---

## File Structure

```
/home/user/daw/
├── zenith-core/                    # Main JUCE application
│   ├── CMakeLists.txt              # CMake build configuration
│   ├── include/                    # Public headers
│   │   ├── Engine.h                # Audio engine interface
│   │   ├── MainWindow.h            # Main window and UI
│   │   ├── ProjectState.h          # Project state management
│   │   ├── TrackAutomationSynchronizer.h  # Automation sync
│   │   └── CommandAPI.h            # JSON command interface
│   ├── src/                        # Implementation files
│   │   ├── Main.cpp                # Application entry point
│   │   ├── MainWindow.cpp          # Main window implementation
│   │   ├── Engine.cpp              # Audio engine implementation
│   │   ├── ProjectState.cpp        # Project state implementation
│   │   ├── TrackAutomationSynchronizer.cpp
│   │   └── CommandAPI.cpp
│   ├── Source/                     # Legacy/ported code
│   │   └── engine/                 # Engine primitives
│   │       ├── Track.h / .cpp      # Track class
│   │       ├── Clip.h / .cpp       # Clip class
│   │       └── MixerChannel.h / .cpp  # Mixer channel
│   ├── tests/                      # Unit tests
│   │   └── ProjectStateTests.cpp   # ProjectState tests
│   ├── build/                      # CMake build output (gitignored)
│   └── README.md                   # Zenith Core README
├── docs/                           # Documentation
│   ├── ARCHITECTURE_OVERVIEW.md    # This file
│   ├── DEVELOPER_WORKFLOW.md       # Development guide
│   ├── Phase13_TrackAutomation_MVP_Summary.md
│   └── ...
├── README.md                       # Project README
├── QUICKSTART.md                   # Quick start guide (outdated)
└── CMakeLists.txt                  # Root CMake (redirects to zenith-core)
```

---

## Technology Stack

### Core Framework

- **JUCE 8.0.9:** Cross-platform audio framework
  - `juce_audio_basics`: Core audio types (AudioBuffer, MIDI)
  - `juce_audio_devices`: Audio I/O (ASIO, CoreAudio, WASAPI)
  - `juce_audio_formats`: File I/O (WAV, AIFF, FLAC, MP3, OGG)
  - `juce_audio_processors`: Plugin hosting (VST3, AU - Phase 2)
  - `juce_audio_utils`: High-level audio components
  - `juce_gui_basics`: UI components (Button, Label, Component)
  - `juce_gui_extra`: Advanced UI (WebBrowserComponent - future)
  - `juce_data_structures`: ValueTree, UndoManager
  - `juce_core`: File I/O, threading, memory

### Language & Standards

- **C++20:** Modern C++ with concepts, ranges (where applicable)
- **CMake 3.22+:** Cross-platform build system
- **FetchContent:** Automatic JUCE dependency fetching

### Audio APIs

- **Windows:**
  - WASAPI (Shared/Exclusive): Native Windows audio (recommended)
  - ASIO: Professional low-latency audio
  - DirectSound: Legacy fallback
  - MME: Legacy fallback

- **macOS:**
  - CoreAudio: Native low-latency audio (automatic)

- **Linux:**
  - ALSA: Native Linux audio
  - JACK: Professional audio routing

### Compiler Support

- **Windows:** Visual Studio 2022 (v143), MSVC 19.3+
- **macOS:** Xcode 14+ (Clang 14+)
- **Linux:** GCC 11+ or Clang 14+

### Dependencies (Managed by JUCE)

- **libFLAC:** FLAC audio codec
- **libOgg/Vorbis:** OGG Vorbis codec
- **libpng:** PNG image loading
- **libjpeg:** JPEG image loading
- **FreeType:** Font rendering

---

## Key Architectural Decisions

### 1. Why Pure JUCE (No Qt/QML/Electron)?

**Decision:** 100% JUCE 8 components, no hybrid stack.

**Rationale:**
- ✅ **Lower CPU overhead:** No Chromium/Qt event loop
- ✅ **Better audio integration:** JUCE designed for real-time audio
- ✅ **Single framework:** Less code complexity, one build system
- ✅ **Proven in industry:** Used by major DAWs (Ableton, FL Studio, Bitwig)
- ✅ **OpenGL acceleration:** Built-in hardware-accelerated rendering

**Trade-offs:**
- ❌ UI is more code-heavy than QML declarative syntax
- ❌ No web tech for complex layouts (but CEF planned for Wingman panel)

---

### 2. Why ValueTree for State Management?

**Decision:** JUCE ValueTree + UndoManager for all project state.

**Rationale:**
- ✅ **Built-in undo/redo:** Free unlimited undo with transactions
- ✅ **Automatic serialization:** toXml() / fromXml() for free
- ✅ **Efficient change notifications:** Only changed properties trigger listeners
- ✅ **Single source of truth:** No state duplication or sync issues
- ✅ **Proven pattern:** Used by major JUCE-based DAWs

**Trade-offs:**
- ❌ Learning curve for ValueTree API
- ❌ Not thread-safe (message thread only)

---

### 3. Why Atomics for Track Controls?

**Decision:** `std::atomic` for volume, pan, mute instead of ValueTree.

**Rationale:**
- ✅ **Lock-free reads:** Audio thread reads without blocking
- ✅ **Simple and fast:** Single atomic load in audio callback
- ✅ **Real-time safe:** No allocations or system calls
- ✅ **Proven pattern:** Industry standard for DAW parameter automation

**Trade-offs:**
- ❌ Manual synchronization between ValueTree and atomics
- ❌ Automation synchronizer adds complexity (but necessary for any DAW)

---

### 4. Why 60 Hz Automation Update Rate?

**Decision:** TrackAutomationSynchronizer runs at 60 Hz (16.67 ms latency).

**Rationale:**
- ✅ **Good enough:** Human perception threshold ~20ms
- ✅ **Low CPU overhead:** Minimal cost compared to audio callback
- ✅ **Smooth visuals:** Matches typical monitor refresh rate
- ✅ **Configurable:** Can be adjusted if needed

**Trade-offs:**
- ❌ Not sample-accurate (but acceptable for most automation)
- ❌ Future: Plugin automation may need higher resolution

---

### 5. Why Windows-First Development?

**Decision:** Focus on Windows (VS2022) for initial hardening.

**Rationale:**
- ✅ **Market share:** Windows dominates DAW market
- ✅ **MMCSS priority:** Windows "Pro Audio" thread scheduling
- ✅ **ASIO/WASAPI:** Excellent low-latency audio support
- ✅ **Easier debugging:** Better tooling (ETW, WPA, Visual Studio)

**Trade-offs:**
- ❌ macOS/Linux support delayed (but JUCE makes porting easy)

---

## Next Steps & Roadmap

### Phase 14: Arranger UI (In Progress)
- Visual timeline with clips
- Automation lanes
- Piano Roll view
- Mixer panel

### Phase 15: Recording Engine
- Audio input capture
- Record to new clips
- Punch in/out
- Latency compensation

### Phase 16: Plugin Hosting
- VST3 hosting (in-process)
- Plugin scanning and management
- Plugin parameter automation
- Out-of-process sandboxing (Phase 17)

### Phase 17: Export/Bounce
- Offline rendering
- Mixdown to WAV/FLAC/MP3
- Stem export
- Real-time export

### Phase 18: MIDI Engine
- MIDI clip editing
- Piano Roll functionality
- Virtual instruments
- MIDI routing

---

## Resources for New Contributors

### Must-Read Documents
1. **Phase13_TrackAutomation_MVP_Summary.md:** Detailed automation implementation
2. **DEVELOPER_WORKFLOW.md:** Build instructions and development practices
3. **JUCE Documentation:** https://docs.juce.com/

### Key JUCE Concepts
- **ValueTree:** https://docs.juce.com/master/classValueTree.html
- **AudioIODeviceCallback:** https://docs.juce.com/master/classAudioIODeviceCallback.html
- **UndoManager:** https://docs.juce.com/master/classUndoManager.html
- **Component:** https://docs.juce.com/master/classComponent.html

### Code Reading Order
1. `src/Main.cpp` - Application entry
2. `include/MainWindow.h` - UI structure
3. `include/Engine.h` - Audio engine interface
4. `include/ProjectState.h` - State management
5. `Source/engine/Track.h` - Track implementation

---

## Glossary

- **DAW:** Digital Audio Workstation
- **JUCE:** Cross-platform C++ framework for audio applications
- **ValueTree:** JUCE's hierarchical data structure with change notifications
- **RT-Safe:** Real-Time Safe - code that runs without blocking on audio thread
- **MMCSS:** Multimedia Class Scheduler Service (Windows audio priority)
- **ASIO:** Audio Stream Input/Output (professional Windows audio)
- **WASAPI:** Windows Audio Session API (native Windows audio)
- **CoreAudio:** macOS native audio API
- **VST3:** Virtual Studio Technology 3 (plugin format)
- **AU:** Audio Units (macOS plugin format)
- **Lock-Free:** Data structures that don't use mutexes (use atomics instead)
- **SPSC:** Single Producer Single Consumer (lock-free queue pattern)

---

## Contact & Support

- **GitHub Issues:** Report bugs and feature requests
- **Discussions:** Architecture questions and design proposals
- **Code Reviews:** All PRs require review before merge

---

**Document Version:** 1.0
**Last Reviewed:** 2025-11-17
**Next Review:** After Phase 14 completion
