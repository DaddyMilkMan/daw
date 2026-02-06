# Zenith DAW Naming Standards

## Overview

This document defines the naming conventions for files, classes, and identifiers in Zenith DAW to ensure consistency and navigability.

---

## File Naming

### Format

```
[DomainPrefix][ComponentName].[extension]
```

### Domain Prefixes

| Domain | Prefix | Module | Example |
|--------|--------|--------|---------|
| Audio Engine | `Engine` | zenith_core | `EngineTransport.h` |
| UI Framework | `UI` | zenith_ui | `UISkiaButton.h` |
| DSP Processing | `DSP` | zenith_dsp | `DSPCompressor.h` |
| Instruments | `Inst` | zenith_core | `InstPolySynth.h` |
| Network | `Net` | zenith_network | `NetCollaborationClient.h` |
| Commands | `Cmd` | zenith_commands | `CmdTrackCommands.h` |
| Effects | `FX` | zenith_core | `FXReverb.h` |

### Rules

1. **Unique Filenames**: No two files can have the same basename
   ```
   ✅ EngineSkiaRenderer.h (zenith_core)
   ✅ UISkiaRenderer.h (zenith_ui)
   ❌ SkiaRenderer.h (ambiguous)
   ```
   
   **Current Status:** Some duplicate filenames exist (migration in progress):
   - `SkiaRenderer.h` in both `engine/rendering/` and `ui/rendering/`
   - `TransportController.h` in both `engine/` and `engine/core/`
   - See `scripts/safe-rename.py --list-approved` for migration plan

2. **PascalCase**: All files use PascalCase
   ```
   ✅ EngineTransportController.h
   ❌ engineTransportController.h
   ❌ engine_transport_controller.h
   ```

3. **Match Class Name**: Filename should match the primary class
   ```cpp
   // File: EngineTransportController.h
   class EngineTransportController { };  // ✅ Matches
   ```

---

## Class Naming

### General Classes

```cpp
// Format: [Domain][Component]
class EngineTransportController { };     // Audio engine transport
class UISkiaButton { };                   // UI button component
class DSPCompressor { };                  // DSP compressor effect
class NetWebSocketClient { };             // Network WebSocket client
```

### Interface Classes

```cpp
// Prefix with 'I' for interfaces
class IAudioEngine { };                   // Engine interface
class ITransportController { };           // Transport interface
class ITrackManager { };                  // Track manager interface
```

### Abstract Base Classes

```cpp
// Prefix with 'Abstract' or use 'Base' suffix
class AbstractInstrument { };             // Base instrument class
class TrackProcessorBase { };             // Base track processor
```

### Test Classes

```cpp
// Format: [Component]Test or Test[Component]
class EngineTransportTest : public ::testing::Test { };
class TestUISkiaButton : public ::testing::Test { };
```

---

## Namespace Conventions

### Top-Level Namespace

All code lives in the `zenith` namespace:

```cpp
namespace zenith {
    // Core code
}

namespace zenith::ui {
    // UI code
}

namespace zenith::dsp {
    // DSP code
}

namespace zenith::net {
    // Network code
}
```

### Module Namespaces

```cpp
namespace zenith {
    // Engine components
    class Engine { };
    class TransportController { };
    
    namespace ui {
        // UI components
        class SkiaButton { };
    }
    
    namespace dsp {
        // DSP components
        class Compressor { };
    }
}
```

---

## Function Naming

### Member Functions

```cpp
class EngineTransportController {
public:
    // Public API: camelCase
    void play();
    void stop();
    void setTempo(double bpm);
    
    // Getters: get[Property]
    double getTempo() const;
    bool isPlaying() const;
    
    // Setters: set[Property]
    void setLoopEnabled(bool enabled);
    
private:
    // Private helpers: camelCase with verb prefix
    void updatePlayhead();
    void processTransportChange();
};
```

### Free Functions

```cpp
// Utility functions: camelCase
namespace zenith::utils {
    double sampleToTime(int samples, double sampleRate);
    std::string formatTime(double seconds);
}
```

---

## Variable Naming

### Member Variables

```cpp
class EngineTransport {
private:
    // Private members: camelCase with trailing underscore
    double tempo_;
    bool isPlaying_;
    juce::int64 playheadPosition_;
    
    // Complex types: descriptive names
    std::unique_ptr<EngineCore> engineCore_;
    std::vector<std::unique_ptr<Track>> tracks_;
    
public:
    // Public members (avoid if possible): no underscore
    double publicTempo;
};
```

### Local Variables

```cpp
void processBlock() {
    // Local variables: camelCase
    double currentTempo = getTempo();
    int numSamples = buffer.getNumSamples();
    
    // Constants: kCamelCase or ALL_CAPS
    const double kMaxTempo = 999.0;
    const int MIN_BUFFER_SIZE = 16;
    
    // Pointers: use 'p' prefix or descriptive
    auto* track = trackManager_.getCurrentTrack();
    auto* audioData = buffer.getWritePointer(0);
}
```

### Parameters

```cpp
void setTempo(double newTempo, bool shouldNotify = true);
void processBlock(juce::AudioBuffer<float>& outputBuffer, 
                  const juce::MidiBuffer& inputMidi);
```

---

## Enum Naming

### Enum Class

```cpp
// Enum class: PascalCase for name, PascalCase for values
enum class TransportState {
    Stopped,
    Playing,
    Recording,
    Paused
};

enum class PlayheadDisplayMode {
    Time,
    Beats,
    Samples
};
```

### Old-Style Enums (Avoid)

```cpp
// Avoid plain enums - use enum class
enum Color {  // ❌ Avoid
    Red,
    Green,
    Blue
};
```

---

## Test Naming

### Test File Names

```cpp
// Format: [Component]Test.cpp
EngineTransportTest.cpp       // ✅
UISkiaButtonTest.cpp          // ✅
transport_test.cpp            // ❌ Wrong case
```

### Test Names

```cpp
// Format: TEST(Component, Scenario_ExpectedResult)
TEST(EngineTransport, GivenStopped_WhenPlayCalled_ThenIsPlaying) {
    // Arrange
    EngineTransport transport;
    
    // Act
    transport.play();
    
    // Assert
    EXPECT_TRUE(transport.isPlaying());
}

TEST(UISkiaButton, Clicked_EmitsCallback) {
    // ...
}
```

---

## Include Guard Naming

```cpp
// Format: MODULE_PATH_FILENAME_H_
#ifndef ZENITH_CORE_ENGINE_TRANSPORT_CONTROLLER_H_
#define ZENITH_CORE_ENGINE_TRANSPORT_CONTROLLER_H_

// ... header content ...

#endif  // ZENITH_CORE_ENGINE_TRANSPORT_CONTROLLER_H_
```

Or use `#pragma once` (preferred):

```cpp
#pragma once  // ✅ Preferred
```

---

## CMake Target Naming

```cmake
# Modules: lowercase with underscores
add_library(zenith_core)
add_library(zenith_ui)
add_library(zenith_dsp)

# Executables: PascalCase
add_executable(ZenithDAW)
add_executable(ZenithTests)

# Test targets: module_name_tests
add_executable(zenith_core_tests)
add_executable(zenith_ui_tests)
```

---

## Examples

### Complete Example

```cpp
// File: modules/zenith_core/engine/EngineTransportController.h

#pragma once

#include <zenith_core/engine/EngineCore.h>

namespace zenith {

/**
 * @brief Controls transport (play/stop/record) for the audio engine
 */
class EngineTransportController {
public:
    enum class State {
        Stopped,
        Playing,
        Recording
    };
    
    EngineTransportController(EngineCore& engine);
    ~EngineTransportController();
    
    // Transport control
    void play();
    void stop();
    void record();
    void togglePlayStop();
    
    // State queries
    State getState() const;
    bool isPlaying() const;
    bool isRecording() const;
    
    // Position
    void setPositionBeats(double beats);
    double getPositionBeats() const;
    
    // Looping
    void setLoopRange(double startBeats, double endBeats);
    void setLoopEnabled(bool enabled);
    bool isLoopEnabled() const;
    
private:
    EngineCore& engine_;
    State currentState_ = State::Stopped;
    double positionBeats_ = 0.0;
    bool loopEnabled_ = false;
    double loopStartBeats_ = 0.0;
    double loopEndBeats_ = 4.0;
    
    void updateEngineState();
};

} // namespace zenith
```

---

## Enforcement

These standards are enforced via:

1. **CI Checks**: `.github/workflows/naming-validation.yml`
2. **Code Review**: All PRs checked against this document
3. **Scripts**: `scripts/rename-duplicates.sh` for automation

---

## Migration Guide

When renaming existing code:

1. Run `scripts/rename-duplicates.sh` to fix duplicate filenames
2. Run `scripts/update-includes.sh` to update includes
3. Run `scripts/fix-includes.py` to normalize include style
4. Build and run tests to verify
5. Commit with message: `refactor: standardize naming per NAMING_STANDARDS.md`
