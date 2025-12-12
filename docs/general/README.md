
```
zenith-core/          → Canonical Zenith DAW application (JUCE 8, C++20)
├── Source/engine/    → Audio engine, tracks, clips, mixer, audio file pool
├── Source/instruments/ → Built-in instruments (ZenithPolySynth, ZenithSampler)
├── src/              → Main application (Engine, MainWindow, ProjectState)
├── include/          → Public headers
└── tests/            → Unit tests

scripts/              → Lua/scripting experiments for future modding
ai-bridge-server/     → Experimental AI/Wingman bridge (Node/WebSocket)
docs/                 → Architecture documentation and design notes
planning/             → Roadmaps, phase plans, feature specs
```

---

## 🗂️ Legacy Code Removal

All older prototypes have been **removed from the active codebase**:
- **VexelDAW-Native/** (donor reference code)
- **src/audio/** (early C++ engine prototype)
- **src/juce-engine/** (placeholder skeleton)
- **src/qt-qml/** (Qt6/QML UI experiment)
- **src/renderer/** (Electron/React prototype)

These are available in git history prior to commit `[Cleanup] Remove legacy projects` if needed for reference.

---

## 🚀 Windows Quick Start

**Want to build Zenith on Windows? It's easy!**

- **Windows 10/11** (64-bit)
- **Visual Studio 2022** with "Desktop development with C++"
- **No WSL, no MSYS2, no package managers required**
- **JUCE is fetched automatically** — no manual setup

**Three-step install:**

```cmd
git clone https://github.com/DaddyMilkMan/zenith-core.git
cd zenith-core
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug -j
```

**Run it:**
```cmd
build\Debug\Zenith.exe
```

**Full Windows installation guide:** [docs/INSTALL_WINDOWS.md](docs/INSTALL_WINDOWS.md)

**Developer workflow (IDE setup, debugging, profiling):** [docs/DEVELOPER_WORKFLOW.md](docs/DEVELOPER_WORKFLOW.md)

---

## 🚀 Building (Cross-platform)

### Prerequisites

- **CMake** 3.22 or higher
- **C++20** compatible compiler:
  - macOS: Xcode 14+ / Clang
  - Windows: Visual Studio 2022 (v143)
  - Linux: GCC 11+ or Clang 14+
- **JUCE 8.0.9** (automatically fetched by CMake)

### Build Instructions

From the repository root:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

The built application will be in:
- **macOS:** `build/zenith-core/ZenithDAW_artefacts/Release/Zenith DAW.app`
- **Windows:** `build/zenith-core/ZenithDAW_artefacts/Release/Zenith DAW.exe`
- **Linux:** `build/zenith-core/ZenithDAW_artefacts/Release/Zenith DAW`

### Run Tests

```bash
cd build
ctest
```

---

## ✅ Current Status

**Canonical JUCE 8 Engine (Phase 1-2 Complete):**
- ✅ RT-safe audio engine with unified render path
- ✅ Track mixdown with pre-allocated buffers (no RT allocations)
- ✅ AudioFilePool for shared audio file caching
- ✅ Lock-free clip snapshots (RCU pattern)
- ✅ Playhead-driven clip rendering with loop support
- ✅ MIDI input routing and recording
- ✅ MIDI clip playback with quantization
- ✅ Instrument system (ZenithPolySynth, ZenithSampler)
- ✅ Automation synchronizer
- ✅ Project state management
- ✅ Unified export path (WAV rendering)

**In Progress:**
- VST3 plugin hosting
- Timeline view with visual clip editing
- Piano roll editor
- Enhanced UI components (arranger view)

**Windows-specific:**
- MMCSS "Pro Audio" thread priority
- WASAPI (Shared/Exclusive) and ASIO support
- Per-monitor DPI correctness

---

## 📚 Documentation

- **Architecture:** See `docs/` for engine design, threading model, RT-safety guidelines
- **Roadmap:** See `planning/` for phase plans and feature timelines
- **Code Organization:** All canonical implementations are in `zenith-core/`

---

## 🎯 Philosophy

**RT-Safety First:** No allocations, locks, or I/O on the audio thread. Ever.

**Clean Architecture:** Single responsibility, explicit dependencies, no globals.

**Moddable by Design:** Lua scripting, extensible clip types, plugin-like architecture.

**AI-Assisted Workflows:** Wingman integration for intelligent assistance (future).

**Best-of-DAW Workflows:** Combining the best from Ableton, FL Studio, Cubase, Bitwig, and Studio One.

---

## 🛠️ Key Features (current & planned)

**Native JUCE 8 UI** with custom ZenithLookAndFeel (dark theme, vectors, subtle animations).

**Wingman (built-in assistant):** Edit/arrange operations with undo-aware diffs (scope expanding).

**Performance foundations:**
- MMCSS "Pro Audio" for the audio thread (Windows)
- Paint-safe UI (no allocations in hot paths), TrackView virtualization
- Per-monitor DPI correctness; Debug HUD for frame timing (planned)

**Moddability (roadmap):**
- Tokenized theme system (live preview)
- Scripting (Lua/JS) sandbox for actions, panels, macros
- Extension SDK for deep hooks (C++/Rust)

---

## 🤝 Contributing

PRs and discussions welcome.
Please keep changes small and scoped (one component or subsystem at a time), and avoid touching audio-thread code unless necessary.

**Coding guidelines:**
- C++20, JUCE conventions
- No allocations in audio callbacks or UI hot paths
- Hook UI actions through ApplicationCommandManager and UndoManager as they land

---

## 📝 License

TBD (likely dual: GPLv3 for open + commercial for closed-source).
Framework licenses apply (JUCE, VST3 SDK, etc.).

---

**Built with ❤️ and JUCE 8**
