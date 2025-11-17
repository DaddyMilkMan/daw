Zenith — Professional Digital Audio Workstation

(formerly “Vexel DAW”)








Overview

Zenith is a professional DAW focused on native performance, modern visuals, and an integrated assistant (Wingman) that helps with editing, arrangement, and session ops.
We’ve migrated away from web UIs and hybrid stacks to a pure C++ / JUCE 8 interface for predictable CPU usage, smooth rendering, and rock-solid real-time behavior.

Goals

Combine the best workflows from major DAWs into one focused environment.

Remove the small, persistent UX pain points reviewers complain about.

Ship a moddable platform with a clean theme system and future scripting/SDK hooks.

Windows Quick Start

**Native Windows • No WSL • No MSYS2 • JUCE Auto-Fetched**

New to Zenith? Get started in minutes:

1. Install Visual Studio 2022 (Desktop development with C++), CMake ≥ 3.22, and Git
2. Clone: git clone https://github.com/DaddyMilkMan/daw.git
3. Build:
cd daw/zenith-core
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release -j
4. Run: .\build\Release\ZenithDAW_artefacts\Release\"Zenith DAW.exe"

Detailed guides:

docs/INSTALL_WINDOWS.md — Step-by-step Windows installation

docs/DEVELOPER_WORKFLOW.md — Development workflow, coding standards, testing

docs/WINDOWS_AUDIO_APIS_GUIDE.md — Audio API selection (ASIO/WASAPI)

No WSL, MSYS2, or vcpkg required. JUCE is automatically fetched by CMake.

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

Native Windows Development • No WSL Required

Compiler/IDE: Visual Studio 2022 (v143), CMake ≥ 3.22

Audio APIs: WASAPI (Shared/Exclusive), ASIO (if available)

Threading: MMCSS "Pro Audio" priority helper (toggle via CMake)

Framework: JUCE 8.0.9 (auto-fetched via CMake FetchContent)

See docs/INSTALL_WINDOWS.md and docs/DEVELOPER_WORKFLOW.md for full setup.

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

Building (Windows)

Quick build (native JUCE application):

cd zenith-core
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release -j


See docs/INSTALL_WINDOWS.md for detailed instructions, troubleshooting, and Visual Studio integration.

Build Options

-DZENITH_ENABLE_MMCSS=ON (default): enable MMCSS "Pro Audio" boost for audio thread

-DZENITH_ENGINE_SEED_DEBUG_TRACKS=OFF: create 8 demo tracks at startup (Debug builds only)

Running

After building, launch the standalone app from your build output (.exe).
Initial tests: Transport controls (play/stop/record), TrackView scrolling/zoom, Sidebar tabs, BPM entry/tap, HiDPI on mixed-scaling monitors.

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
