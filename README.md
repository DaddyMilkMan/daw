Zenith — Professional Digital Audio Workstation

(formerly “Vexel DAW”)








Overview

Zenith is a professional DAW focused on native performance, modern visuals, and an integrated assistant (Wingman) that helps with editing, arrangement, and session ops.
We’ve migrated away from web UIs and hybrid stacks to a pure C++ / JUCE 8 interface for predictable CPU usage, smooth rendering, and rock-solid real-time behavior.

Goals

Combine the best workflows from major DAWs into one focused environment.

Remove the small, persistent UX pain points reviewers complain about.

Ship a moddable platform with a clean theme system and future scripting/SDK hooks.

Architecture

Current UI Stack: 100% native JUCE 8 Components (C++).
No Electron. No Qt/QML. No embedded browsers.

┌───────────────────────────────────────────────────────────┐
│                     Zenith UI (JUCE 8)                    │
│  • Custom LookAndFeel (dark theme, vector/SVG, animations)│
│  • Components: TopBar, Sidebar, TrackView, TransportBar   │
│  • High-DPI (Per-Monitor V2) aware                        │
└───────────────▲───────────────────────────────────────────┘
                │ clean callbacks / commands
┌───────────────┴───────────────────────────────────────────┐
│                 Engine & Platform Layer                   │
│  • AudioDeviceManager, transport, session state           │
│  • Windows: MMCSS (“Pro Audio”) thread priority           │
│  • Lock-free comms for meters/events (WIP)                │
└───────────────────────────────────────────────────────────┘


Windows-first: We’re focusing on Windows (VS2022) for early hardening; macOS/Linux will follow.

Key Features (current & planned)

Native JUCE 8 UI with custom ZenithLookAndFeel (dark theme, vectors, subtle animations).

Wingman (built-in assistant): edit/arrange operations with undo-aware diffs (scope expanding).

Performance foundations:

MMCSS “Pro Audio” for the audio thread (Windows).

Paint-safe UI (no allocations in hot paths), TrackView virtualization (in progress).

Per-monitor DPI correctness; Debug HUD for frame timing (planned).

Best-of DAW workflows (roadmap):

Ableton-style clip/launch view, FL-style piano roll ergonomics,

Studio One-style Arranger/Scratchpad,

Cubase-style articulation maps, Bitwig-like modular mindset.

Moddability (roadmap):

Tokenized theme system (live preview),

Scripting (Lua/JS) sandbox for actions, panels, macros,

Extension SDK for deep hooks (C++/Rust).

Windows Support

Compiler/IDE: Visual Studio 2022 (v143), CMake ≥ 3.22

Audio APIs: WASAPI (Shared/Exclusive), ASIO (if available)

Threading: MMCSS “Pro Audio” priority helper (toggle via CMake)

Project Structure (current)
zenith-core/
├─ CMakeLists.txt
├─ Source/
│  ├─ ui/
│  │  ├─ ZenithLookAndFeel.h/.cpp
│  │  ├─ MainComponent.h/.cpp
│  │  ├─ TopBar.h/.cpp
│  │  ├─ Sidebar.h/.cpp
│  │  ├─ TrackView.h/.cpp
│  │  └─ TransportBar.h/.cpp
│  ├─ win/
│  │  ├─ WinRtAudioPriority.h/.cpp     # MMCSS RAII helper
│  │  └─ (Windows-specific helpers)
│  └─ (engine/, audio/, etc. as they land)
└─ docs/
   └─ windows/
      └─ mmcss_audio_priority.md

Building

Quick Start (All Platforms)

```bash
./build.sh          # Linux/macOS
build.bat           # Windows
```

Or use CMake directly:

```bash
# Navigate to zenith-core directory
cd zenith-core

# Configure (JUCE will be auto-fetched)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release -j
```

Prerequisites

- CMake ≥ 3.22
- C++20 compiler:
  - **Windows**: Visual Studio 2022 (Desktop development with C++)
  - **macOS**: Xcode 13+ or Clang
  - **Linux**: GCC 10+ or Clang 12+
- JUCE 8.0.9 (auto-fetched by CMake)


CMake Options (zenith-core)

- `ZENITH_ENGINE_SEED_DEBUG_TRACKS=ON/OFF`: Create 8 demo tracks at startup (Debug builds)

Running

```bash
./run.sh            # Linux/macOS
run.bat             # Windows
```

Or run directly:
- **Windows**: `zenith-core/build/ZenithDAW_artefacts/Release/ZenithDAW.exe`
- **macOS**: `zenith-core/build/ZenithDAW_artefacts/Release/ZenithDAW.app`
- **Linux**: `zenith-core/build/ZenithDAW_artefacts/Release/ZenithDAW`

Initial tests: Transport controls (play/stop), audio device initialization, track automation, CPU monitoring.

Performance Notes

The audio thread registers with MMCSS (“Pro Audio”) at startup (Windows).

UI avoids allocations in paint(); heavy geometry/text is cached.

TrackView is being virtualized to render only visible rows/columns (reduce overdraw).

A Debug HUD for frame time / paints-per-second is planned to help profile scroll/zoom.

For deep profiling, we use ETW/WPA and PresentMon (docs coming).

Wingman (Built-in Assistant)

Wingman is embedded—not a chat bolt-on. Early focus:

Undo-aware edit/arrange commands with clear diffs,

Session-graph awareness (tracks, regions, routing),

Human-readable change logs inside the project.

Future: mix-assist (gain staging/loudness targets) once the mixer lands.

Moddability & Extensibility (Roadmap)

Theme tokens (colours, radii, spacing, fonts) with live reload.

Scripting sandbox (Lua/JS) for actions, macros, and custom panels.

Extension SDK for deep integration (routing, editors, controller scripts).

Target: more moddable than Reaper or Ableton, with safer packaging and clearer APIs.

Roadmap (abridged)

W3: Windows audio settings panel (WASAPI Shared/Exclusive, sample rate, buffer).

Mixer: channel strips, meters, sends; lock-free metering pipeline.

Piano Roll: fast draw, scale helpers, velocity lanes.

Session/Launcher: clip-based creation.

Plugin Hosting: VST3 (AU later on macOS), crash isolation plan.

Interchange: project I/O groundwork; DAW-friendly export later.

Contributing

PRs and discussions welcome.
Please keep changes small and scoped (one component or subsystem at a time), and avoid touching audio-thread code unless necessary.

Coding guidelines:

C++17, JUCE conventions.

No allocations in audio callbacks or UI hot paths.

Hook UI actions through ApplicationCommandManager and UndoManager as they land.

License

TBD (likely dual: GPLv3 for open + commercial for closed-source).
Framework licenses apply (JUCE, VST3 SDK, etc.).

Project Status

Active development — Windows-first hardening in progress.
Old Electron/React code is archived; Qt/QML references were removed to reflect the current native JUCE architecture.
