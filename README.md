# Zenith DAW — Professional Digital Audio Workstation

**Formerly "Vexel DAW"**

[![Version](https://img.shields.io/badge/version-0.1.0-blue.svg)](https://github.com/DaddyMilkMan/daw)
[![JUCE](https://img.shields.io/badge/JUCE-8.0.9-orange.svg)](https://juce.com/)
[![C++](https://img.shields.io/badge/C++-20-00599C.svg)](https://en.cppreference.com/)
[![CMake](https://img.shields.io/badge/CMake-3.22+-064F8C.svg)](https://cmake.org/)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey.svg)](https://github.com/DaddyMilkMan/daw)

---

## Overview

Zenith is a **professional Digital Audio Workstation** built with pure C++ and JUCE 8, focused on:

- **Native Performance:** 100% C++ JUCE 8 components — no Electron, Qt, or web overhead
- **Real-Time Safety:** Lock-free audio processing with strict thread safety guarantees
- **Modern Architecture:** ValueTree-based state management with full undo/redo support
- **Integrated AI:** Wingman assistant for editing, arrangement, and session operations (roadmap)

**Key Milestone:** We've completed the migration from hybrid architectures (Electron/Qt/QML) to a fully native JUCE 8 implementation, delivering predictable CPU usage, smooth 60 FPS rendering, and rock-solid real-time audio behavior.

---

## Documentation

### For New Contributors

- **[Architecture Overview](docs/ARCHITECTURE_OVERVIEW.md)** — System architecture, data flow, threading model, and module breakdown
- **[Developer Workflow](docs/DEVELOPER_WORKFLOW.md)** — Build instructions, development practices, debugging, and profiling

### Implementation Details

- **[Phase 13: Track Automation MVP](docs/Phase13_TrackAutomation_MVP_Summary.md)** — Complete automation system documentation
- **[Migration History](docs/MIGRATION_FROM_ELECTRON.md)** — Evolution from Electron to pure JUCE

---

## Quick Start

### Prerequisites

| Tool | Version | Platform |
|------|---------|----------|
| **CMake** | 3.22+ | All |
| **C++ Compiler** | C++20 | MSVC 19.3+ / GCC 11+ / Clang 14+ |
| **JUCE** | 8.0.9 | Auto-fetched by CMake |

**Platform-Specific:**
- **Windows:** Visual Studio 2022 (Desktop C++ workload)
- **macOS:** Xcode 14+ (`xcode-select --install`)
- **Linux:** `sudo apt install build-essential cmake` + JUCE dependencies (see Developer Workflow)

### Build and Run

```bash
# Clone repository
git clone <repository-url>
cd daw/zenith-core

# Configure (Debug)
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build/Debug -j

# Run
./build/Debug/ZenithDAW_artefacts/Debug/ZenithDAW  # Linux/macOS
build\Debug\ZenithDAW_artefacts\Debug\ZenithDAW.exe  # Windows
```

**For detailed instructions, see [Developer Workflow](docs/DEVELOPER_WORKFLOW.md).**

---

## Goals

Zenith aims to combine the best workflows from major DAWs into one focused, moddable environment:

- ✅ **Remove UX pain points** that reviewers consistently complain about in existing DAWs
- ✅ **Deliver predictable performance** through native C++ and careful real-time audio design
- 🚧 **Provide a moddable platform** with clean theme system and future scripting/SDK hooks
- 🚧 **Integrate AI assistance** (Wingman) for editing, arrangement, and session operations

---

## Architecture

### Current Stack

**100% Pure JUCE 8** — No Electron. No Qt/QML. No embedded browsers.

```
┌───────────────────────────────────────────────────────────┐
│                     Zenith UI (JUCE 8)                    │
│  • Custom LookAndFeel (dark theme, vector/SVG, animations)│
│  • Components: MainWindow, Transport, Status Display      │
│  • High-DPI (Per-Monitor V2) aware                        │
└───────────────▲───────────────────────────────────────────┘
                │ Clean callbacks / commands
┌───────────────┴───────────────────────────────────────────┐
│                 Engine & Platform Layer                   │
│  • AudioDeviceManager (ASIO, WASAPI, CoreAudio)           │
│  • ProjectState (ValueTree + UndoManager)                 │
│  • Windows: MMCSS "Pro Audio" thread priority             │
│  • Lock-free Track atomics for automation playback        │
└───────────────────────────────────────────────────────────┘
```

### Development Focus

**Windows-first:** Initial focus on Windows (VS2022) for early hardening with MMCSS audio priority; macOS/Linux support follows.

**For full architecture details, see [Architecture Overview](docs/ARCHITECTURE_OVERVIEW.md).**

---

## Key Features

### Current (Phase 13 Complete)

- ✅ **Native JUCE 8 UI** with modern dark theme
- ✅ **Audio Engine** with transport controls (play/stop/record)
- ✅ **Track System** with volume, pan, mute, solo, and arming
- ✅ **Clip System** for audio/MIDI clip playback with fades and looping
- ✅ **Track Automation** (volume, pan, mute) with beat-synchronized playback
- ✅ **ProjectState** (ValueTree) with full undo/redo support
- ✅ **Command API** (JSON) for Wingman AI integration
- ✅ **Real-Time Safety** via lock-free atomics and strict thread separation

### Planned (Roadmap)

- 🚧 **Arranger UI** (Phase 14): Visual timeline, automation lanes, Piano Roll
- 🚧 **Recording Engine** (Phase 15): Audio input capture, punch in/out, latency compensation
- 🚧 **Plugin Hosting** (Phase 16): VST3/AU hosting, parameter automation, sandboxing
- 🚧 **Export/Bounce** (Phase 17): Offline rendering, mixdown to WAV/FLAC/MP3
- 🚧 **MIDI Engine** (Phase 18): MIDI clip editing, Piano Roll, virtual instruments
- 🚧 **Wingman AI** (Phase 19): Embedded AI assistant for editing and arrangement

---

## Windows Support

### Compiler & Tools

- **Visual Studio 2022** (v143 toolset)
- **CMake** ≥ 3.22
- **C++20** standard

### Audio APIs

- ✅ **WASAPI** (Shared/Exclusive): Native Windows audio, recommended for most users
- ✅ **ASIO**: Professional low-latency audio for supported interfaces
- ✅ **DirectSound**: Legacy fallback
- ✅ **MME**: Legacy fallback

### Threading

- ✅ **MMCSS "Pro Audio" priority**: Enabled by default for audio thread (toggle via CMake)
- ✅ **Lock-free audio processing**: No allocations, locks, or system calls on audio thread

---

## macOS Support (Planned)

- **Xcode 14+** (Clang compiler)
- **CoreAudio**: Native low-latency audio (automatic)
- **Universal Binaries**: arm64 + x86_64 support
- **Code Signing**: Required for distribution

---

## Linux Support (Planned)

- **GCC 11+** or **Clang 14+**
- **ALSA**: Native Linux audio
- **JACK**: Professional audio routing
- **Package**: AppImage or Flatpak distribution

---

## Project Structure

```
daw/
├── zenith-core/                    # Main JUCE application
│   ├── CMakeLists.txt              # CMake build configuration
│   ├── include/                    # Public headers
│   │   ├── Engine.h                # Audio engine
│   │   ├── MainWindow.h            # UI components
│   │   ├── ProjectState.h          # ValueTree state management
│   │   ├── TrackAutomationSynchronizer.h  # Automation sync
│   │   └── CommandAPI.h            # JSON command interface
│   ├── src/                        # Implementation
│   │   ├── Main.cpp                # Application entry
│   │   ├── Engine.cpp
│   │   ├── MainWindow.cpp
│   │   ├── ProjectState.cpp
│   │   ├── TrackAutomationSynchronizer.cpp
│   │   └── CommandAPI.cpp
│   ├── Source/                     # Engine primitives
│   │   └── engine/
│   │       ├── Track.h / .cpp      # Track class
│   │       ├── Clip.h / .cpp       # Clip class
│   │       └── MixerChannel.h / .cpp
│   ├── tests/                      # Unit tests
│   │   └── ProjectStateTests.cpp
│   └── build/                      # Build output (gitignored)
├── docs/                           # Documentation
│   ├── ARCHITECTURE_OVERVIEW.md    # System architecture
│   ├── DEVELOPER_WORKFLOW.md       # Development guide
│   └── Phase13_TrackAutomation_MVP_Summary.md
├── README.md                       # This file
└── CMakeLists.txt                  # Root CMake (redirects to zenith-core)
```

---

## Building

### Windows

```powershell
# Configure
cmake -S zenith-core -B zenith-core/build/Debug -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build zenith-core/build/Debug -j

# Run
zenith-core\build\Debug\ZenithDAW_artefacts\Debug\ZenithDAW.exe
```

### macOS / Linux

```bash
# Configure
cmake -S zenith-core -B zenith-core/build/Debug -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build zenith-core/build/Debug -j

# Run
./zenith-core/build/Debug/ZenithDAW_artefacts/Debug/ZenithDAW
```

### CMake Options

```bash
# Enable MMCSS "Pro Audio" priority (Windows, default ON)
-DZENITH_ENABLE_MMCSS=ON

# Create 8 demo tracks at startup (Debug builds only)
-DZENITH_ENGINE_SEED_DEBUG_TRACKS=ON

# Enable link-time optimization (Release builds)
-DZENITH_LTCG=ON

# Treat warnings as errors (CI builds)
-DZENITH_WERROR_CI=ON
```

---

## Performance Notes

### Audio Thread (Real-Time Safe)

- ✅ **No allocations:** All buffers pre-allocated in `prepareToPlay()`
- ✅ **No locks:** All cross-thread communication via `std::atomic`
- ✅ **No system calls:** No file I/O, logging, or network on audio thread
- ✅ **MMCSS priority:** Windows "Pro Audio" thread scheduling (if enabled)

### UI Thread

- ✅ **60 FPS target:** All UI repaints optimized for smooth rendering
- ✅ **Cached geometry:** Heavy text and path rendering cached
- ✅ **Virtualized views:** TrackView renders only visible rows/columns (planned)

### Memory

- ✅ **Fixed-size allocations:** ValueTree nodes allocated once, reused
- ✅ **Shared immutable data:** Audio buffers use copy-on-write where possible
- ✅ **Minimal overhead:** Pure C++, no garbage collection

---

## Wingman (AI Assistant)

**Status:** Roadmap (Phase 19)

Wingman is an embedded AI assistant, not a chat bolt-on. Early focus:

- 🚧 **Undo-aware editing:** Commands integrated with UndoManager for safe rollback
- 🚧 **Session-graph awareness:** Understands tracks, regions, routing, automation
- 🚧 **Human-readable change logs:** Shows what changed after each command
- 🚧 **Mix assistance:** Gain staging, loudness targets (post-mixer implementation)

---

## Moddability & Extensibility

**Status:** Roadmap

- 🚧 **Theme tokens:** Colors, radii, spacing, fonts with live reload
- 🚧 **Scripting sandbox:** Lua/JS for actions, macros, custom panels
- 🚧 **Extension SDK:** Deep integration (routing, editors, controller scripts)
- 🚧 **Goal:** More moddable than Reaper or Ableton, safer packaging, clearer APIs

---

## Contributing

PRs and discussions welcome! Please keep changes small and scoped (one component or subsystem at a time).

### Coding Guidelines

- **C++20** with JUCE conventions (CamelCase, `jassert`, `JUCE_DECLARE_NON_COPYABLE`)
- **No allocations** in audio callbacks or UI hot paths
- **Thread safety comments:** Mark all code as MESSAGE THREAD or AUDIO THREAD
- **Undo integration:** Hook UI actions through `UndoManager`
- **Documentation:** Update `docs/` if architecture changes

**For detailed guidelines, see [Developer Workflow](docs/DEVELOPER_WORKFLOW.md).**

---

## License

**TBD** (likely dual-license: GPLv3 for open-source + commercial for closed-source)

Framework licenses apply:
- **JUCE:** Commercial license required for closed-source products (see https://juce.com/legal)
- **VST3 SDK:** MIT License (free to use, no royalties)

---

## Project Status

**Active Development** — Windows-first hardening in progress.

### Recent Milestones

- ✅ **Phase 0:** Pure JUCE 8 refactor (removed Electron/Qt/QML)
- ✅ **Phase 13:** Track automation MVP (volume, pan, mute)
- 🚧 **Phase 14:** Arranger UI (timeline, automation lanes, Piano Roll)

### Archived Experiments

- Old Electron/React code: Removed
- Qt/QML prototypes: Removed (see `docs/MIGRATION_FROM_ELECTRON.md` for history)

---

## Resources

### Official Documentation

- **JUCE:** https://docs.juce.com/
- **VST3 SDK:** https://steinbergmedia.github.io/vst3_dev_portal/
- **CoreAudio:** https://developer.apple.com/documentation/coreaudio
- **WASAPI:** https://learn.microsoft.com/en-us/windows/win32/coreaudio/

### Community

- **JUCE Forum:** https://forum.juce.com/
- **Audio Developer Conference:** https://audio.dev/
- **KVR Audio DSP Forum:** https://www.kvraudio.com/forum/viewforum.php?f=33

### Books

- *Designing Audio Effect Plugins in C++* by Will Pirkle
- *The Audio Programming Book* by Richard Boulanger
- *Real-Time C++* by Christopher Kormanyos

---

## Contact

- **GitHub Issues:** Bug reports and feature requests
- **Discussions:** Architecture questions and design proposals
- **Code Reviews:** All PRs require review before merge

---

**Version:** 0.1.0
**Last Updated:** 2025-11-17
**Maintained By:** Zenith DAW Team
