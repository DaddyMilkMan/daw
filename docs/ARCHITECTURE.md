# Zenith DAW - Architecture Overview

**Author**: Sophia "The Architect" Chen - Operation Polish  
**Last Updated**: February 2026

**License:** [AGPL v3](../LICENSE) - Open source, contributions welcome

---

## System Architecture

Zenith DAW follows a modular, event-driven architecture with clear separation between the audio engine, UI, and integration layers.
The codebase follows the **Citadel** directory structure pattern to ensure separation of concerns and scalability.

## Directory Structure (Citadel)

The repository is now split into app shell + reusable modules:

*   **`apps/desktop/Source/`**: Desktop entry point, platform glue, and thin app wiring.
*   **`modules/zenith_core/`**: Engine, DSP, instruments, plugins, and core utilities.
*   **`modules/zenith_ui/`**: Skia UI framework, views, and renderer integration.
*   **`modules/zenith_network/`**: Collaboration and API clients.
*   **`modules/zenith_commands/`**: Command API for AI integration.
*   **`services/ai/`**: Python backend + agents.

```
┌─────────────────────────────────────────────────────────────┐
│                        Main Window                          │
│  ┌───────────┐  ┌──────────────┐  ┌──────────────────────┐ │
│  │ Transport │  │  Main Layout │  │   Right Side Panel   │ │
│  │    Bar    │  │  (Session/   │  │  (Wingman + Pads)    │ │
│  └───────────┘  │   Arranger)  │  └──────────────────────┘ │
│                 └──────────────┘                            │
│  ┌────────────────────────────────────────────────────────┐ │
│  │            Bottom Bar (Piano + Mixer)                  │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
          │                    │                    │
          ▼                    ▼                    ▼
  ┌──────────────┐    ┌───────────────┐    ┌──────────────┐
  │  ProjectState│    │ Audio Engine  │    │  Command API │
  │  (ValueTree) │◄───┤  (Real-time)  │◄───┤  (AI Bridge) │
  └──────────────┘    └───────────────┘    └──────────────┘
          │                    │
          ▼                    ▼
  ┌──────────────┐    ┌───────────────┐
  │ Undo Manager │    │ Audio Devices │
  │   (History)  │    │  (ASIO/WASAPI)│
  └──────────────┘    └───────────────┘
```

---

## Core Components

### 1. **ProjectState** (Data Layer)
- Central `juce::ValueTree` holding all project data
- Tracks, clips, automation, parameters
- Thread-safe with listeners for UI updates
- Undo/redo via `juce::UndoManager`

### 2. **Engine** (Audio Processing)
- Real-time audio rendering
- Track graph with sends/returns
- Plugin hosting (VST3)
- MIDI routing and processing
- Runs on audio thread (lock-free where possible)

### 3. UI Components (Presentation Layer)
- **Skia**: Exclusive rendering engine (GPU-accelerated)
- **Theme System**: `ZenithTheme` for consistent styling
- **Responsive**: Adapts to window resizing

### 4. **Instruments** (Built-in Processors)
- `ZenithPolySynth`: Subtractive synthesis
- `ZenithSampler`: Multi-sample playback
- Registered via `InstrumentRegistry`
- Support preset system

### 5. **Command API** (AI Integration)
- Natural language processing via Grok
- Executes commands on `ProjectState`
- Undo-aware operations
- Asynchronous execution

---

## Data Flow

### Audio Path
```
AudioCallback (External)
  └─► Engine::processAudioAndMidi()
       ├─► Track::processBlock()
       │    ├─► Clip::getNextAudioBlock()
       │    └─► PluginHost::processBlock()
       └─► MixerChannel::processBlock()
            └─► Output to device
```

### UI Update Path
```
User Input (Mouse/Keyboard)
  └─► Component::mouseDown/keyPressed()
       └─► ProjectState::addTrack/addClip() [via UndoableAction]
            └─► ValueTree::setProperty()
                 └─► ValueTree::Listener::valueTreePropertyChanged()
                      └─► Component::repaint()
```

### AI Command Path
```
User Voice/Text
  └─► WingmanPanel::sendCommand()
       └─► GrokAPIClient::processRequest()
            └─► GrokDAWController::executeCommand()
                 └─► CommandAPI::executeJSONCommand()
                      └─► ProjectState mutation
                           └─► UI update (via listeners)
```

---

## Threading Model

### Audio Thread (Real-time, lock-free)
- `Engine::processAudioAndMidi()`
- Must complete within buffer time (e.g., 5ms @ 128 samples, 48kHz)
- No allocations, no blocking

### Message Thread (UI)
- All JUCE components
- `ProjectState` mutations (via undo manager)
- File I/O, plugin loading

### Background Threads
- Audio file streaming (`AudioFilePool`)
- Plugin scanning
- AI API calls
- Preset generation

**Synchronization:**
- `juce::MessageManager` for cross-thread calls
- `AbstractFifo` for audio thread → UI communication
- `jassert` for thread safety checks (debug only)

---

## Plugin Architecture

### VST3 Hosting
```
PluginHost
  └─► juce::KnownPluginList (scanned plugins)
       └─► juce::AudioPluginInstance (loaded plugin)
            └─► Track::insertPlugin()
                 └─► Saved in ProjectState as ValueTree
```

### Built-in Instruments
```
InstrumentRegistry
  ├─► ZenithPolySynth (ID: "zenith.polysynth")
  ├─► ZenithSampler (ID: "zenith.sampler")
  └─► [Future instruments]
       └─► Track::setInstrument(id)
            └─► Instantiated on audio thread
```

---

## File Format

### Project File (`.zenith`)
- XML-based `ValueTree` serialization
- Contains:
  - Tracks (audio/MIDI/instrument)
  - Clips (references to audio files)
  - Automation envelopes
  - Plugin states (binary blobs)
  - Mixer settings

### Audio Files
- Referenced by path (relative to project)
- Pooled loading via `AudioFilePool`
- Supports WAV, AIFF, FLAC, MP3

### Presets
- JSON format for instruments
- Stored in `Content/Presets/`
- Per-instrument folders

---

## Build System

### CMake Configuration
- **Generator**: Visual Studio 2022
- **Platform**: x64
- **Build Types**: Debug, Release
- **Options**:
  - `ZENITH_USE_SKIA=ON` - Enable Skia rendering (Required)
  - `CMAKE_PREFIX_PATH` - vcpkg path for dependencies

### Dependencies (via vcpkg)
- `skia` - Graphics rendering
- `libpng`, `libjpeg`, `zlib` - Image codecs
- `harfbuzz`, `icu` - Text rendering
- JUCE (fetched via FetchContent)

---

## Performance Considerations

### Audio Processing
- **Target Latency**: <10ms round-trip
- **Buffer Sizes**: 128, 256, 512 samples (user-configurable)
- **CPU Usage**: Multi-threaded track processing (future)

### UI Rendering
- **Skia**: GPU-accelerated (Metal on macOS, Direct3D 12 on Windows, Vulkan on Linux), 60 FPS target
- **Dirty Regions**: Only repaint changed areas

### Memory Usage
- **Audio Files**: Streamed from disk, not fully loaded
- **Plugin State**: Lazy loading of presets
- **Undo History**: Limited to 100 actions (configurable)

---

## Extension Points

### Adding a New Instrument
1. Create `MyInstrument.h/cpp` in `Source/instruments/`
2. Inherit from `juce::AudioProcessor`
3. Register in `RegisterBuiltInInstruments.cpp`
4. Add presets to `Content/Presets/my-instrument/`

### Adding a New UI Component
1. Create in `Source/ui/` or `Source/ui/skia/`
2. Inherit from `SkiaComponent`
3. Use `ZenithTheme` for styling
4. Add to parent layout in `MainLayoutComponent`

### Adding a New Command
1. Add command definition in `CommandAPI.cpp`
2. Implement handler in `executeJSONCommand()`
3. Create undo action if modifying `ProjectState`
4. Document in `INSTRUMENT_COMMAND_API.md`

---

## Future Architecture Plans

### Planned Improvements
- **Multi-threaded Audio**: Parallel track processing
- **GPU-accelerated DSP**: Compute shaders for effects
- **Cloud Sync**: Project backup and collaboration
- **Plugin Sandboxing**: Isolate plugin crashes
- **Modular UI**: User-customizable layouts

---

## References

- **JUCE Documentation**: https://docs.juce.com
- **Skia Documentation**: https://skia.org/docs
- **VST3 SDK**: https://steinbergmedia.github.io/vst3_dev_portal/

---

*This architecture is a living document. Contributions and improvements welcome!*
