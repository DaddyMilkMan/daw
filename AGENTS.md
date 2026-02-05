# AGENTS.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview

Zenith DAW is a professional Digital Audio Workstation built with C++20 and JUCE 8, featuring Skia GPU-accelerated rendering, AI-powered creative assistance (Grok integration), and real-time collaboration.

## Build Commands

```bash
# Configure and build (Release)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run the application
./build/Zenith\ DAW

# Build with tests
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build --target ZenithDAWTests

# Run tests
./build/ZenithDAWTests_artefacts/Release/ZenithDAWTests

# Run specific test category
./build/ZenithDAWTests_artefacts/Release/ZenithDAWTests AudioEngine

# Run single test
./build/ZenithDAWTests_artefacts/Release/ZenithDAWTests AudioEngine.TrackProcessing

# Debug build with sanitizers
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build
LSAN_OPTIONS=suppressions=../lsan.supp ./build/ZenithDAWTests
```

## Critical Thread Safety Rules

**Audio Thread (~1ms budget) - NEVER:**
- `malloc`, `new`, `delete`, `std::vector::push_back` (any allocation)
- `std::mutex`, `juce::CriticalSection` (any lock)
- `std::cout`, `printf`, `DBG()` (any logging)
- File I/O, `juce::MessageManager` calls
- Direct `ValueTree` access

**Audio Thread - ONLY:**
- `std::atomic` reads (lock-free)
- Pre-allocated buffer access
- Arithmetic operations

**UI Thread:**
- Use Skia rendering (not `juce::Graphics` for new code)
- `ValueTree` read/write allowed
- No blocking operations >16ms

**Cross-thread communication:**
- Use `juce::MessageManager::callAsync()` from background threads
- Use `std::atomic` for audio thread state synchronization
- Use `AbstractFifo` for audio thread → UI communication

## Wayland Context Loss (Critical for Linux)

OpenGL context can be lost at ANY time on Wayland. Must handle in every render frame:

```cpp
void renderFrame(int width, int height) {
    // 1. Validate context (REQUIRED)
    if (!grContext_ || grContext_->abandoned()) {
        recreateContext();
        return;
    }
    
    // 2. Query FBO every frame (NEVER cache)
    GLint fbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    
    // 3. Recreate surface on resize
    if (width != lastWidth_ || height != lastHeight_) {
        recreateSurface(width, height);
    }
}
```

## Architecture

### Directory Structure (Citadel Pattern)
```
apps/desktop/Source/
├── engine/       # Audio engine, tracks, clips, ProjectState
├── ui/           # User interface components (Skia-based)
├── ai/           # AI/ML features, Grok integration
├── dsp/          # Signal processing
├── instruments/  # Built-in synths (ZenithPolySynth, ZenithSampler)
├── commands/     # Command API for AI integration
├── network/      # Collaboration and API clients
├── rendering/    # Skia context management
└── tests/        # Unit and integration tests
```

### State Management
All application state lives in `ProjectState` (`juce::ValueTree`). Audio thread uses `std::atomic` copies synchronized with the ValueTree:

```cpp
// UI thread writes:
void setTempo(double t) {
    state_.setProperty(ID_TEMPO, t, &undoManager_);
    tempo_.store(t);  // Sync atomic
}

// Audio thread reads:
double getTempo() const {
    return tempo_.load();  // Lock-free
}
```

### Data Flow
- **Audio**: `Engine::processAudioAndMidi()` → `Track::processBlock()` → `Clip::getNextAudioBlock()`
- **UI Updates**: User Input → `Component` → `ProjectState` (via UndoableAction) → `ValueTree::Listener` → `repaint()`
- **AI Commands**: `WingmanPanel` → `GrokAPIClient` → `CommandAPI::executeJSONCommand()` → `ProjectState` mutation

## Code Style

- C++20 standard, 4-space indentation
- `PascalCase` for classes, `camelCase` for functions/variables
- `camelCase_` trailing underscore for member variables
- `Zenith::` namespace (capital Z)
- All variables must be initialized with `{}`
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`), no raw pointers
- Use RAII for all resources

## Testing Infrastructure

Tests use JUCE UnitTest framework. Test executables:
- `ZenithDAWTests` - Main unit test runner
- `WCETStressTests` - Real-time constraint validation
- `MixerChannelBenchmark`, `WavetableBenchmark` - Performance benchmarks

Agents in `agents/` directory provide CI automation:
- `TestingAgent` - Test discovery and execution
- `FuzzingAgent` - Robustness testing for DSP/MIDI/plugins
- `SecurityAgent` - Vulnerability scanning
- `LintingAgent` - Code quality checks

## Key Files

- `apps/desktop/Source/engine/Engine.h` - Audio engine core
- `apps/desktop/Source/engine/ProjectState.h` - Central state management
- `apps/desktop/Source/ui/` - All UI components
- `apps/desktop/Source/rendering/` - Skia integration
- `docs/ARCHITECTURE.md` - Full architecture documentation
- `docs/THREADING_MODEL.md` - Thread safety details
- `aiagentsreadthis` - Extended rules and anti-patterns reference

## Performance Targets

- Audio latency: <5ms roundtrip
- UI: 60fps minimum
- Frame budget: <16ms @ 60Hz
- Cold start: <2 seconds
- Memory (idle): <500MB
