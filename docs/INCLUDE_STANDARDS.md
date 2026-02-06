# Zenith DAW Include Standards

## Overview

This document defines the include conventions for Zenith DAW to ensure fast compilation, avoid circular dependencies, and make code navigation easier.

---

## General Rules

### 1. Always Use Absolute Paths

```cpp
// ✅ Good - Absolute path with angle brackets
#include <zenith_core/engine/Engine.h>
#include <zenith_ui/components/UISkiaButton.h>
#include <zenith_dsp/dsp/Compressor.h>

// ❌ Bad - Relative paths
#include "../../../engine/Engine.h"
#include "../../dsp/Dither.h"

// ❌ Bad - Ambiguous paths
#include "Engine.h"  // Which Engine.h?
```

### 2. Use Angle Brackets for Project Includes

```cpp
// ✅ Good - Angle brackets for project code
#include <zenith_core/engine/Engine.h>

// ❌ Bad - Quotes for project code
#include "zenith_core/engine/Engine.h"
```

### 3. Use Quotes Only for Same-Directory Includes

```cpp
// ✅ Good - Same directory, private header
#include "EnginePrivate.h"

// In Engine.cpp
#include "Engine.h"  // Corresponding header (same dir)
```

---

## Include Order

Within each file, includes should be ordered as follows:

### For `.cpp` Files

```cpp
// 1. Corresponding header (always first!)
#include <zenith_core/engine/Engine.h>

// 2. Other project headers (alphabetical)
#include <zenith_core/dsp/Dither.h>
#include <zenith_core/dsp/MasterLimiter.h>
#include <zenith_core/engine/TransportController.h>

// 3. JUCE headers (alphabetical)
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>

// 4. System headers (alphabetical)
#include <atomic>
#include <memory>
#include <vector>
```

### For `.h` Files

```cpp
#pragma once

// 1. Forward declarations when possible
namespace juce {
    class AudioBuffer;
    class MidiBuffer;
}

// 2. Minimal includes for compilation
#include <juce_core/juce_core.h>

// 3. Other project headers only if needed
#include <zenith_core/engine/EngineConstants.h>

// 4. System headers
#include <memory>
```

---

## Include Style

### Full Module Path

```cpp
// Format: <module_name/path/to/File.h>

#include <zenith_core/engine/Engine.h>
#include <zenith_core/engine/core/EngineCore.h>
#include <zenith_core/dsp/Dither.h>
#include <zenith_core/instruments/ZenithPolySynth.h>

#include <zenith_ui/components/UISkiaButton.h>
#include <zenith_ui/theme/ZenithTheme.h>
#include <zenith_ui/views/session/SessionView.h>
```

### CMake Configuration

To make absolute includes work, CMake is configured with:

```cmake
# In each module's CMakeLists.txt
target_include_directories(zenith_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules>
        $<INSTALL_INTERFACE:include>
)
```

This allows:
```cpp
#include <zenith_core/engine/Engine.h>  // Resolves to modules/zenith_core/engine/Engine.h
```

---

## Forward Declarations

### When to Use Forward Declarations

Use forward declarations instead of includes when:
- Only pointers or references to the type are used
- The type is used in function signatures but not called
- The type is a return type or parameter type

```cpp
// ✅ Good - Forward declaration
#pragma once

namespace zenith {

class Engine;  // Forward declaration

class TransportController {
public:
    explicit TransportController(Engine& engine);  // Reference only
    
    void doSomething();
    
private:
    Engine& engine_;  // Reference member
};

} // namespace zenith
```

### When to Include

Include the full header when:
- You need to call methods on the object
- You need to know the size (member by value)
- You inherit from the class
- You use the type in a template

```cpp
// ❌ Must include - calling method
#pragma once

namespace zenith {

class Engine;  // Forward declaration NOT enough

class TransportController {
public:
    void play() {
        engine_.play();  // ❌ Error - incomplete type
    }
    
private:
    Engine& engine_;
};

} // namespace zenith

// ✅ Correct version
#pragma once

#include <zenith_core/engine/Engine.h>  // Must include

namespace zenith {

class TransportController {
public:
    void play() {
        engine_.play();  // ✅ Works
    }
    
private:
    Engine& engine_;
};

} // namespace zenith
```

---

## Pimpl Idiom

For reducing compile-time dependencies, use the Pimpl (Pointer to Implementation) idiom:

```cpp
// Engine.h
#pragma once

#include <memory>

namespace zenith {

class Engine {
public:
    Engine();
    ~Engine();
    
    void play();
    void stop();
    
private:
    class Impl;                    // Forward declaration
    std::unique_ptr<Impl> pImpl_;  // Implementation pointer
};

} // namespace zenith

// Engine.cpp
#include <zenith_core/engine/Engine.h>

#include <zenith_core/engine/EngineCore.h>
#include <zenith_core/engine/TransportController.h>
// ... other includes only in cpp

namespace zenith {

class Engine::Impl {
public:
    EngineCore core;
    TransportController transport;
    // ... implementation details
};

Engine::Engine() : pImpl_(std::make_unique<Impl>()) {}
Engine::~Engine() = default;

void Engine::play() {
    pImpl_->transport.play();
}

} // namespace zenith
```

---

## Circular Dependencies

### Avoid Circular Includes

```cpp
// ❌ BAD - Circular dependency
// File: Engine.h
#include <zenith_core/engine/TransportController.h>

class Engine {
    TransportController transport_;  // Engine includes Transport
};

// File: TransportController.h
#include <zenith_core/engine/Engine.h>

class TransportController {
    Engine& engine_;  // Transport includes Engine
};
```

### Solution: Forward Declaration + Interface

```cpp
// ✅ GOOD - No circular dependency
// File: Engine.h
#pragma once

namespace zenith {

class TransportController;  // Forward declaration

class Engine {
public:
    Engine();
    ~Engine();
    
    TransportController& getTransport();
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace zenith

// File: TransportController.h
#pragma once

namespace zenith {

class Engine;  // Forward declaration

class TransportController {
public:
    explicit TransportController(Engine& engine);
    
private:
    Engine& engine_;
};

} // namespace zenith

// Implementation in .cpp files with full includes
```

---

## Include What You Use (IWYU)

### Principle

Every file should explicitly include all headers it directly uses.

```cpp
// ✅ Good - Explicit includes
#include <vector>
#include <string>

class MyClass {
    std::vector<std::string> items_;  // Uses both vector and string
};

// ❌ Bad - Relies on transitive includes
#include <vector>  // string comes through here (fragile!)

class MyClass {
    std::vector<std::string> items_;  // string not explicitly included
};
```

### Tools

Use clang-tidy to check:

```bash
clang-tidy -checks='misc-include-cleaner' modules/zenith_core/engine/Engine.cpp
```

---

## Precompiled Headers

For faster compilation, use precompiled headers for commonly included files:

```cpp
// pch.h (Precompiled Header)
#pragma once

// JUCE headers
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

// Standard library
#include <vector>
#include <memory>
#include <string>

// Project common
#include <zenith_core/engine/EngineConstants.h>
```

Configure in CMake:

```cmake
target_precompile_headers(zenith_core
    PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/desktop/Source/pch/pch.h
)
```

---

## Examples

### Complete Header Example

```cpp
// File: modules/zenith_core/engine/EngineTransportController.h

#pragma once

// Forward declarations
namespace juce {
    class AudioBuffer;
    class MidiBuffer;
}

namespace zenith {

// Forward declarations
class EngineCore;
class TransportState;

// Project includes (only what's needed)
#include <zenith_core/engine/EngineConstants.h>

// System includes
#include <atomic>
#include <functional>

namespace zenith {

/**
 * @brief Controls transport state for the audio engine
 */
class EngineTransportController {
public:
    enum class State {
        Stopped,
        Playing,
        Recording
    };
    
    explicit EngineTransportController(EngineCore& core);
    ~EngineTransportController();
    
    // Transport control
    void play();
    void stop();
    void record();
    
    // State queries
    State getState() const;
    bool isPlaying() const;
    bool isRecording() const;
    
    // Position
    void setPositionBeats(double beats);
    double getPositionBeats() const;
    
private:
    EngineCore& core_;
    std::atomic<State> state_{State::Stopped};
    double positionBeats_ = 0.0;
};

} // namespace zenith
```

### Complete Implementation Example

```cpp
// File: modules/zenith_core/engine/EngineTransportController.cpp

// 1. Corresponding header
#include <zenith_core/engine/EngineTransportController.h>

// 2. Other project headers (alphabetical)
#include <zenith_core/engine/EngineCore.h>
#include <zenith_core/engine/EngineEvent.h>
#include <zenith_core/utils/Logger.h>

// 3. JUCE headers (alphabetical)
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

// 4. System headers (alphabetical)
#include <cmath>

namespace zenith {

EngineTransportController::EngineTransportController(EngineCore& core)
    : core_(core) {
}

EngineTransportController::~EngineTransportController() = default;

void EngineTransportController::play() {
    state_.store(State::Playing);
    core_.notifyTransportChanged();
}

void EngineTransportController::stop() {
    state_.store(State::Stopped);
    core_.notifyTransportChanged();
}

// ... rest of implementation

} // namespace zenith
```

---

## Enforcement

These standards are enforced via:

1. **CI Checks**: `.github/workflows/include-hygiene.yml`
2. **Scripts**: `scripts/fix-includes.py` (auto-fix)
3. **Code Review**: All PRs checked against this document

---

## Quick Reference

| Scenario | Include Style | Example |
|----------|--------------|---------|
| Same directory | `"File.h"` | `#include "EnginePrivate.h"` |
| Other project file | `<module/path/File.h>` | `<zenith_core/dsp/Dither.h>` |
| JUCE header | `<juce_module/header.h>` | `<juce_core/juce_core.h>` |
| System header | `<header>` | `<vector>` |
| Forward declaration | `class Name;` | `class Engine;` |
