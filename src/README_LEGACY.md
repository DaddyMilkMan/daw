# ⚠️ LEGACY PROTOTYPES – DO NOT EDIT

This directory contains **abandoned prototypes** from early development phases.

## What's Here

- **`audio/`** – Early C++ audio engine prototype (namespace `vexel`)
- **`juce-engine/`** – Placeholder skeleton with TODOs (never completed)
- **`qt-qml/`** – Qt6/QML UI experiment (different framework)

## Status

❌ **NOT COMPILED** by the active build system
❌ **NOT MAINTAINED**
📚 **Kept for historical reference only**

## Active Development

✅ **All active development happens in:** `/zenith-core/`

**Canonical implementations:**
- Audio Engine: `zenith-core/include/Engine.h`
- Audio File Pool: `zenith-core/Source/engine/AudioFilePool.*`
- Clip Rendering: `zenith-core/Source/engine/Clip.*`
- Track Processing: `zenith-core/Source/engine/Track.*`
- Import Audio UI: `zenith-core/src/MainWindow.cpp`

## Build System

⚠️ **Root `/CMakeLists.txt` builds legacy Qt6 prototype** (DO NOT USE)
✅ **Use `zenith-core/CMakeLists.txt` for active JUCE-based DAW**

---

**Last Updated:** 2025-11-13 (Phase 1.4 complete)
