# Zenith DAW Architecture Documentation

**Comprehensive Technical Documentation for Building a Commercial-Grade DAW**

---

## Overview

This documentation suite provides research-backed, source-cited guidance for building **Zenith DAW**, a professional Digital Audio Workstation using:

- **C++20 + JUCE 8.0.9** for the audio engine and core UI (~85%)
- **HTML/CSS + TypeScript (CEF)** for the Wingman AI panel (~10%)
- **C/SIMD kernels** for DSP hotspots (~5%)

All documents include **official documentation links**, **community best practices**, and **actionable implementation checklists**.

---

## Documents

### 📘 01. JUCE Framework Guide
**File:** [`tech-briefs/01-juce-framework-guide.md`](tech-briefs/01-juce-framework-guide.md)

**Purpose:** Comprehensive overview of JUCE for DAW development

**Topics:**
- JUCE modules (`juce_audio_basics`, `juce_audio_devices`, `juce_dsp`, `juce_gui_basics`)
- `ValueTree` + `UndoManager` for project state management
- Audio thread vs message thread: hard rules
- OpenGL renderer performance caveats
- CMake setup best practices
- Plugin hosting with `AudioProcessorGraph`

**Deliverables:**
- One-page tech brief with sources
- Working `CMakeLists.txt`
- C++ skeleton files: `Main.cpp`, `Engine.h/cpp`, `ProjectState.h/cpp`, `MainWindow.h`

**Key Takeaway:** JUCE is the industry-standard framework for professional audio applications, providing a complete audio stack with proven real-time safety.

---

### 🌐 02. Web Embedding Decision
**File:** [`tech-briefs/02-web-embedding-decision.md`](tech-briefs/02-web-embedding-decision.md)

**Purpose:** Evaluate CEF vs WebView2 vs WKWebView for the Wingman AI panel

**Topics:**
- Feature comparison (custom protocols, DevTools, sandboxing, footprint)
- Platform coverage (CEF: all platforms; WebView2: Windows-only; WKWebView: macOS-only)
- JavaScript bridge patterns (JSON-over-WebSocket recommended)
- Security model (localhost binding, origin checks, token auth)
- JUCE + CEF integration sketch

**Deliverables:**
- Trade-off table
- JUCE + CEF integration code stubs
- Risk matrix with mitigations

**Key Takeaway:** Use **CEF** for cross-platform consistency. WebView2/WKWebView only if willing to maintain dual implementations.

---

### 🎛️ 03. VST3 and AU Hosting Guide
**File:** [`tech-briefs/03-vst3-au-hosting-guide.md`](tech-briefs/03-vst3-au-hosting-guide.md)

**Purpose:** Implement VST3 and Audio Unit plugin hosting

**Topics:**
- **Phase 1:** In-process hosting (quick bring-up, single-process)
- **Phase 2:** Out-of-process sandboxing (crash protection, isolation)
- Official SDK documentation (Steinberg VST3 SDK, Apple AU)
- VST3 parameter automation (IComponent & IEditController)
- macOS codesigning and sandboxing considerations

**Deliverables:**
- Phase 1 implementation checklist (plugin scanning, loading, audio graph integration)
- Phase 2 implementation plan (subprocesses, IPC, crash recovery)
- Code stubs: `PluginHost.h/cpp` with thread safety comments

**Key Takeaway:** Start with in-process hosting (Phase 1). Add sandboxing (Phase 2) only after core functionality is stable.

---

### 🎚️ 04. Audio Driver Latency Guide
**File:** [`tech-briefs/04-audio-driver-latency-guide.md`](tech-briefs/04-audio-driver-latency-guide.md)

**Purpose:** Configure low-latency audio I/O with ASIO, CoreAudio, and WASAPI

**Topics:**
- **ASIO** (Windows): Industry standard, lowest latency (<5ms)
- **CoreAudio** (macOS): Native, stable, unified API
- **WASAPI** (Windows): Native, shared/exclusive modes, low latency (Windows 10+)
- Recommended buffer sizes by workflow (32-64 for recording, 256-512 for mixing)
- Device selection flow (pseudo-UI logic)
- Hot-plug handling

**Deliverables:**
- Latency tuning guide for Win/macOS
- Device selection flow (C++ pseudo-code)
- QA testing checklist for glitch testing at 32/64-sample buffers

**Key Takeaway:** Use **ASIO on Windows**, **CoreAudio on macOS** for professional audio. Target **64-128 samples** for recording, **256-512** for mixing.

---

---

### 🔒 06. Audio Thread Safety Policy
**File:** [`tech-briefs/06-audio-thread-safety-policy.md`](tech-briefs/06-audio-thread-safety-policy.md)

**Purpose:** Establish hard rules for audio callback and lock-free patterns

**Topics:**
- NEVER on audio thread: memory allocation, mutex locks, system calls
- SAFE on audio thread: fixed-size operations, atomics, pre-allocated buffers
- Lock-free patterns:
  - Pattern 1: Simple parameters (`std::atomic`)
  - Pattern 2: Lock-free FIFO (SPSC queue)
  - Pattern 3: Double-buffering (atomic pointer swap)
  - Pattern 4: JUCE `ValueTree` listeners
- Complete parameter system example
- Testing real-time safety (TSan, debug checks)

**Deliverables:**
- One-page policy with "NEVER" and "SAFE" lists
- Lock-free pattern examples (C++ code)
- Complete `ParameterManager` implementation

**Key Takeaway:** The audio callback has a **hard 2-3ms deadline**. Any blocking operation will cause glitches. Use lock-free patterns for all inter-thread communication.

---

### 📦 07. Packaging & Licensing Checklist
**File:** [`tech-briefs/07-packaging-licensing-checklist.md`](tech-briefs/07-packaging-licensing-checklist.md)

**Purpose:** Release checklist for commercial distribution

**Topics:**
- **JUCE licensing:** Commercial license required for closed-source products
- **VST3 SDK licensing:** MIT license (free to use, no royalties)
- **macOS:** Code signing + notarization (Developer ID, Hardened Runtime, timestamping)
- **Windows:** Code signing (EV certificate recommended for SmartScreen)
- Installer creation (DMG/PKG for macOS, EXE/MSIX for Windows)
- VST trademark usage guidelines

**Deliverables:**
- Complete release checklist with step-by-step instructions
- Cost estimate ($780-$1,380/year for licenses and certificates)
- Verification commands for codesigning and notarization

**Key Takeaway:** Budget **~$1,000/year** for JUCE license, Apple Developer Program, and Windows code signing certificate. Notarization is **mandatory** for macOS distribution.

---

## Code Templates

All code templates are in [`code-templates/`](code-templates/):

| **File** | **Description** |
|----------|-----------------|
| [`CMakeLists.txt`](code-templates/CMakeLists.txt) | JUCE project setup with audio modules |
| [`Main.cpp`](code-templates/Main.cpp) | Application entry point with graceful shutdown |
| [`Engine.h`](code-templates/Engine.h) | Audio engine with `AudioProcessorGraph` |
| [`Engine.cpp`](code-templates/Engine.cpp) | Audio callback implementation (real-time safe) |
| [`ProjectState.h`](code-templates/ProjectState.h) | `ValueTree` + `UndoManager` state management |
| [`ProjectState.cpp`](code-templates/ProjectState.cpp) | Project state implementation |
| [`MainWindow.h`](code-templates/MainWindow.h) | Main application window stub |

**Usage:**
1. Copy templates to your `Source/` directory
2. Customize for your project (rename, add features)
3. Follow thread safety rules (see Document 06)

---

## Quick Start Guide

### 1. Clone JUCE

```bash
git clone https://github.com/juce-framework/JUCE.git
cd JUCE
git checkout 8.0.9
```

### 2. Create Project

```bash
mkdir ZenithDAW
cd ZenithDAW
cp ../daw/docs/code-templates/CMakeLists.txt .
mkdir Source
cp ../daw/docs/code-templates/*.cpp ../daw/docs/code-templates/*.h Source/
```

### 3. Configure & Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### 4. Run

```bash
./ZenithDAW
```

---

## Recommended Reading Order

**For New Engineers:**
1. Document 01: JUCE Framework Guide (understand the foundation)
2. Document 06: Audio Thread Safety Policy (critical for all audio code)
3. Document 04: Audio Driver Latency Guide (configure I/O correctly)
4. Document 03: VST3 and AU Hosting Guide (add plugin support)

**For Product Managers:**
1. Document 07: Packaging & Licensing Checklist (understand costs and legal requirements)
2. Document 02: Web Embedding Decision (understand Wingman AI architecture)

**For UI Designers:**
1. Document 01: JUCE Framework Guide (understand UI constraints)
2. Document 02: Web Embedding Decision (understand web tech is isolated to AI panel)
3. ARCHITECTURE.md (understand JUCE + Skia rendering approach)

---

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│  Zenith DAW (C++ / JUCE)                                        │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Core UI (JUCE Components)                                │   │
│  │  - Timeline / Arrangement (zoomable)                     │   │
│  │  - Mixer (meters, inserts, sends)                        │   │
│  │  - Piano Roll                                            │   │
│  │  - Inspector / Browser                                   │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Audio Engine (Real-Time)                                 │   │
│  │  - AudioProcessorGraph (mixer, routing)                  │   │
│  │  - Plugin Hosting (VST3, AU)                             │   │
│  │  - Audio I/O (ASIO, CoreAudio, WASAPI)                   │   │
│  │  - MIDI Processing                                       │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Project State (ValueTree + UndoManager)                  │   │
│  │  - Tracks, Clips, Automation, Settings                   │   │
│  │  - Save/Load (XML serialization)                         │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Wingman AI Panel (CEF / React + TS)                      │   │
│  │  - Chat interface                                        │   │
│  │  - Command preview/diff                                  │   │
│  │  - WebSocket bridge to native (JSON)                     │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

---

## Technology Stack Summary

| **Layer** | **Technology** | **Percentage** | **Purpose** |
|-----------|---------------|----------------|-------------|
| **Audio Engine** | C++20 + JUCE 8.0.9 | ~60% | Real-time audio processing, plugin hosting |
| **Core UI** | JUCE Components | ~25% | Timeline, mixer, piano roll, browser |
| **Wingman AI Panel** | CEF + React/TS | ~10% | AI chat interface (isolated from audio) |
| **DSP Kernels** | C/SIMD | ~5% | Hotspot optimizations (FFT, filters) |

**Total Lines of Code (Estimated):** ~150,000-200,000 LOC for a commercial DAW

---

## Contributing

### Documentation Standards

All documents must include:
- ✅ **Official documentation links** (not blog posts or Stack Overflow)
- ✅ **Direct quotes** with citations
- ✅ **Actionable checklists** (not just theory)
- ✅ **Code examples** (with thread safety comments)
- ✅ **Sources section** at the end

### Code Standards

All code must follow:
- ✅ **Real-time safety** (no allocations/locks on audio thread)
- ✅ **JUCE coding style** (CamelCase, `jassert`, `JUCE_DECLARE_NON_COPYABLE`)
- ✅ **Thread safety comments** (mark audio thread vs message thread)
- ✅ **Pre-allocation** (use fixed-size buffers, `prepareToPlay()`)

---

## Additional Resources

### Official Documentation
- JUCE: https://docs.juce.com/
- VST3 SDK: https://steinbergmedia.github.io/vst3_dev_portal/
- CoreAudio: https://developer.apple.com/documentation/coreaudio
- WASAPI: https://learn.microsoft.com/en-us/windows/win32/coreaudio/

### Community
- JUCE Forum: https://forum.juce.com/
- Audio Developer Conference: https://audio.dev/
- KVR Audio DSP Forum: https://www.kvraudio.com/forum/viewforum.php?f=33

### Books
- *Designing Audio Effect Plugins in C++* by Will Pirkle
- *The Audio Programming Book* by Richard Boulanger
- *Real-Time C++* by Christopher Kormanyos

---

## License

This documentation is provided for internal use by the Zenith DAW development team.

**Software Licenses:**
- JUCE: Commercial license required (see Document 07)
- VST3 SDK: MIT License (free to use)
- CEF: BSD 3-Clause License (free to use)

---

## Version History

| **Version** | **Date** | **Changes** |
|-------------|----------|-------------|
| 1.0 | 2025-11-10 | Initial release: All 7 documents completed |

---

**Last Updated:** 2025-11-10
**Maintained By:** Zenith DAW Architecture Team
**Contact:** [Your Email/Slack Channel]

---

## Next Steps

1. **Read Document 01** (JUCE Framework Guide) to understand the foundation
2. **Copy code templates** to your project and customize
3. **Follow Document 06** (Audio Thread Safety Policy) for all audio code
4. **Implement Phase 1 of Document 03** (VST3/AU Hosting)
5. **Test at low latency** using Document 04 (Audio Driver Guide)
6. **Prepare for release** using Document 07 (Packaging & Licensing)

Good luck building Zenith DAW! 🎵🚀
