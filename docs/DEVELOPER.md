# Developer Guide

## Code Organization

### Source Structure

```
apps/desktop/Source/
├── Main.cpp                    # Application entry point
├── ai_client/                  # C++ client for Python AI service
├── browser/                    # Browser and marketplace integration
├── platform/                   # OS-specific code paths
└── tests/                      # Test files

modules/
├── zenith_core/                # Engine, DSP, instruments, plugins, utils
│   ├── engine/                 # Core audio engine
│   ├── dsp/                    # Signal processing
│   ├── instruments/            # Built-in instruments
│   ├── plugins/                # Internal plugins
│   └── utils/                  # Cross-cutting utilities
├── zenith_ui/                  # Skia UI components
│   ├── ui/                     # UI framework, panels, views
│   └── rendering/              # Skia renderer integration
├── zenith_network/             # Collaboration and network services
└── zenith_commands/            # Command API for AI integration

services/
└── ai/                         # Python backend and agents
```

## Threading Model

### Audio Thread (Real-time)
- `Engine::audioDeviceIOCallback()` runs here
- **Rules:**
  - No memory allocation
  - No locks/mutexes
  - No logging
  - No file I/O
- Use `std::atomic` for cross-thread communication
- Pre-allocate all buffers

### Message Thread (UI)
- All JUCE components run here
- `ProjectState` mutations happen here
- File I/O, plugin loading

### Background Threads
- Audio file streaming
- Plugin scanning
- AI API calls
- Export rendering

## State Management

All project state lives in `ProjectState` using JUCE's `ValueTree`:

```cpp
// Reading state
auto tracks = projectState.getTracksNode();
for (auto track : tracks) {
    auto name = track.getProperty("name").toString();
}

// Modifying state (always on message thread)
projectState.addTrack(TrackType::Audio, "New Track");

// Listening for changes
class MyComponent : public juce::ValueTree::Listener {
    void valueTreePropertyChanged(juce::ValueTree& tree, 
                                   const juce::Identifier& property) override {
        // React to state changes
    }
};
```

## Adding Features

### New Instrument

1. Create `modules/zenith_core/instruments/MyInstrument.h/cpp`
2. Inherit from `juce::AudioProcessor`
3. Register in `modules/zenith_core/instruments/RegisterBuiltInInstruments.cpp`:
```cpp
registry.registerInstrument("zenith.myinstrument", 
    []() { return std::make_unique<MyInstrument>(); });
```

### New UI Component

1. Create in `modules/zenith_ui/ui/`
2. Inherit from `SkiaComponent` for GPU rendering:
```cpp
class MyPanel : public SkiaComponent {
public:
    void renderSkia(SkCanvas& canvas) override {
        // Draw with Skia
    }
};
```

### New Command (for AI integration)

1. Add to `modules/zenith_commands/commands/CommandAPI.cpp`:
```cpp
if (command == "myCommand") {
    // Execute command
    return Result::ok();
}
```

## Testing

Tests are in `apps/desktop/Source/tests/`:

```cpp
// Example test
class MyTest : public juce::UnitTest {
public:
    MyTest() : UnitTest("MyTest") {}
    
    void runTest() override {
        beginTest("Basic functionality");
        expect(someFunction() == expectedValue);
    }
};

static MyTest myTest;
```

Run tests:
```bash
cmake --build build --target ZenithDAWTests
./build/ZenithDAWTests_artefacts/Release/ZenithDAWTests
```

## Code Style

- C++20 standard
- 4-space indentation
- `camelCase` for functions and variables
- `PascalCase` for classes
- `UPPER_SNAKE_CASE` for constants
- Doxygen comments for public APIs

## Debugging

### Audio Issues
- Check `ZenithLogger` output
- Use `RTSafetyChecks.h` assertions in debug builds
- Profile with system audio tools (e.g., `pw-top` on Linux)

### UI Issues
- Enable Skia debug overlay
- Check for missing `repaint()` calls
- Verify component hierarchy

### Memory Issues
- Build with sanitizers:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address"
```

## Performance Guidelines

1. **Audio thread**: Keep processing under buffer duration (e.g., 2.6ms @ 128 samples, 48kHz)
2. **UI**: Target 60 FPS, use dirty region repainting
3. **Memory**: Stream large audio files, don't load entirely
4. **Plugins**: Process in parallel when possible

## UX Guidance (User Experience)

Focus on user workflows and responsiveness alongside UI polish:

1. **State-driven UX**: Route user actions through `ProjectState` so undo/redo and UI updates stay consistent.
2. **Predictable interactions**: Use standard DAW behaviors for selection, dragging, snapping, and transport controls.
3. **Clear feedback**: Provide visible status for long operations (plugin scans, exports, AI calls).
4. **Error handling**: Prefer friendly, actionable messages over silent failure.
5. **Accessibility & scale**: Plan for UI scaling and keyboard shortcuts early; avoid fixed-size assumptions.
