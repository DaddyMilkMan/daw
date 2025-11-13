# Zenith DAW

**Professional native DAW built with JUCE 8 and modern C++20**

Zenith is a high-performance digital audio workstation focused on clean architecture, RT-safe audio processing, deep moddability, and AI-assisted workflows (Wingman).

---

## 🏗️ Project Layout

```
zenith-core/          → Canonical Zenith DAW application (JUCE 8, C++20)
├── Source/engine/    → Audio engine, tracks, clips, mixer, audio file pool
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

These are available in git history prior to commit `[Cleanup] Remove legacy projects` if needed for reference.

---

## 🚀 Building

### Prerequisites

- **CMake** 3.22 or higher
- **C++20** compatible compiler:
  - macOS: Xcode 14+ / Clang
  - Windows: Visual Studio 2019+ / MSVC
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

## ✅ Current Status (Phase 1 Complete)

**Phase 1.4 (2025-11-13):**
- ✅ RT-safe audio engine with track mixdown
- ✅ Atomic playhead tracking with loop support
- ✅ AudioFilePool for pre-loaded audio file caching
- ✅ Playhead-driven clip rendering (parameter-based timing)
- ✅ Import Audio UI (FileChooser → AudioFilePool → Track + Clip)
- ✅ Pre-allocated buffers (no RT allocations)

**Next (Phase 2):**
- Lock-free clip list (replace clipsLock with RCU)
- MIDI clip playback and piano roll
- VST3 plugin hosting
- Recording (audio + MIDI)
- Timeline view with visual clip editing

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

**AI-Assisted Workflows:** Wingman integration for intelligent assistance (Phase 3).

---

## 📝 License

[License TBD - Update with actual license]

---

**Built with ❤️ and JUCE 8**
