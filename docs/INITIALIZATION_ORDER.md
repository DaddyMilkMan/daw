# C++ Initialization Order and Singleton Pattern Documentation

## Overview

This document describes the initialization patterns, singleton usage, and critical initialization order requirements in the Zenith DAW C++ codebase. Understanding these patterns is crucial for maintaining robust, thread-safe code without lazy initialization anti-patterns.

## Singleton Pattern Strategy

### Meyer's Singleton (Recommended)

All singletons in Zenith DAW use the **Meyer's Singleton** pattern (also known as the "Meyers Singleton" or "Static Local Variable Singleton"). This pattern is:

- **Thread-safe** since C++11 (guaranteed by the standard)
- **Automatically destroyed** at program exit (proper RAII)
- **Lazy-initialized** but without the dangers of manual lazy initialization
- **No manual memory management** required

#### Pattern Example:

```cpp
class MyManager {
public:
    static MyManager& getInstance() {
        static MyManager instance;  // Thread-safe, initialized on first access
        return instance;
    }
    
    // Delete copy/move constructors
    MyManager(const MyManager&) = delete;
    MyManager& operator=(const MyManager&) = delete;
    
private:
    MyManager() { /* Initialize here */ }
    ~MyManager() { /* Cleanup here */ }
};
```

### Anti-Patterns to Avoid

❌ **DON'T use raw pointer singletons:**
```cpp
// BAD - Not thread-safe, manual memory management
static MyManager* gInstance = nullptr;
MyManager& getInstance() {
    if (gInstance == nullptr)
        gInstance = new MyManager();
    return *gInstance;
}
```

❌ **DON'T use static unique_ptr with lazy initialization:**
```cpp
// BAD - Static initialization order fiasco risk
static std::unique_ptr<MyManager> instance;
static std::mutex mutex;

MyManager& getInstance() {
    std::lock_guard<std::mutex> lock(mutex);
    if (!instance)
        instance = std::make_unique<MyManager>();
    return *instance;
}
```

## Critical Singletons and Their Initialization

### 1. ZenithLogger (`engine/ZenithLogger.h`)

**Purpose:** Centralized logging system
**Initialization:** Automatic on first use (Meyer's singleton)
**Dependencies:** None - can be used very early

**Best Practice:**
```cpp
// Initialize early in main() to ensure logging is available
ZenithLogger::makeGlobal();  // Sets as JUCE's global logger
```

**Notes:**
- File logger is created in constructor
- Thread-safe for use from any thread
- Should be initialized before other components that may log

### 2. FontManager (`ui/design-system/FontManager.h`)

**Purpose:** Loads and caches custom fonts (Inter, JetBrains Mono)
**Initialization:** Automatic on first use (Meyer's singleton)
**Dependencies:** File system access, font files in Resources/fonts

**Best Practice:**
```cpp
// Access early to ensure fonts are loaded before UI creation
auto& fontManager = FontManager::getInstance();
if (!fontManager.isInitialized()) {
    // Handle missing fonts gracefully
}
```

**Critical Notes:**
- Fonts are loaded from Resources/fonts in constructor
- Must complete before any UI components request fonts
- Failure is non-fatal but will result in fallback fonts

### 3. ZenithLookAndFeel (`ui/design-system/ZenithLookAndFeel.h`)

**Purpose:** Custom UI styling and rendering
**Initialization:** Automatic on first use (Meyer's singleton)
**Dependencies:** None (uses design system constants)

**Best Practice:**
```cpp
// Set as default look and feel early in UI initialization
juce::LookAndFeel::setDefaultLookAndFeel(&ZenithLookAndFeel::getInstance());
```

**Notes:**
- Sets JUCE component colors in constructor
- Thread-safe for access but should be set on message thread

### 4. RealTimeGarbageCollector (`engine/RealTimeGarbageCollector.h`)

**Purpose:** Deferred deletion for real-time audio thread safety
**Initialization:** Automatic on first use (Meyer's singleton)
**Dependencies:** JUCE MessageManager (for timer)

**Best Practice:**
```cpp
// Access early to ensure timer starts before audio threads
auto& gc = RealTimeGarbageCollector::getInstance();
// Cleanup at shutdown:
gc.ensureClean();
```

**Critical Notes:**
- Starts a 100ms timer in constructor if MessageManager exists
- Audio threads can safely defer object deletion
- Call `ensureClean()` before program exit for proper cleanup

### 5. Settings (`Settings.h`)

**Purpose:** Global application settings
**Initialization:** Automatic on first use (Meyer's singleton)
**Dependencies:** File system for persistent storage

**Best Practice:**
```cpp
// Load settings early in application startup
auto& settings = Settings::getInstance();
settings.load();  // Explicit load required!
```

**Critical Notes:**
- `getInstance()` creates the singleton but does NOT load settings
- Must explicitly call `load()` to read from disk
- Call `save()` when settings change

### 6. AIResponseCache (`ai/AIResponseCache.h`)

**Purpose:** Caches AI API responses to reduce API calls
**Initialization:** Automatic on first use (Meyer's singleton)
**Dependencies:** File system for SQLite database

**Notes:**
- Database initialized in constructor
- Thread-safe for concurrent access
- Can be disabled via `setEnabled(false)`

### 7. InternalPluginFormat (`plugins/InternalPluginFormat.h`)

**Purpose:** Hosts internal Zenith plugins (EQ, Compressor, etc.)
**Initialization:** Automatic on first use (Meyer's singleton)
**Dependencies:** Plugin implementations

**Best Practice:**
```cpp
// Register with plugin host early
auto& format = InternalPluginFormat::getInstance();
// Plugins are registered in constructor
```

**Notes:**
- Built-in plugins are registered in constructor
- Must be registered with JUCE AudioPluginFormatManager

### 8. GlobalUndoRedo (`ui/system/UndoRedoSystem.h`)

**Purpose:** Global undo/redo system and manager
**Initialization:** Automatic on first use (Meyer's singleton)
**Dependencies:** None

**Notes:**
- Provides both `UndoRedoSystem` and `UndoRedoManager`
- Thread-safe access
- Separate instances for different concerns

## Initialization Best Practices

### Application Startup Sequence

Recommended initialization order in `main()`:

```cpp
int main(int argc, char* argv[]) {
    // 1. JUCE initialization
    juce::ScopedJuceInitialiser_GUI initializer;
    
    // 2. Logging (earliest possible)
    zenith::ZenithLogger::makeGlobal();
    
    // 3. Settings
    auto& settings = zenith::Settings::getInstance();
    settings.load();
    
    // 4. Font loading (before UI)
    auto& fontManager = zenith::design::FontManager::getInstance();
    
    // 5. Look and Feel (before UI)
    juce::LookAndFeel::setDefaultLookAndFeel(
        &zenith::ZenithLookAndFeel::getInstance()
    );
    
    // 6. Garbage collector (before audio)
    auto& gc = zenith::RealTimeGarbageCollector::getInstance();
    
    // 7. Create main application window
    auto mainWindow = std::make_unique<MainWindow>();
    
    // ... Run application ...
    
    // Shutdown sequence
    mainWindow.reset();
    gc.ensureClean();
    settings.save();
    
    return 0;
}
```

### Thread Safety Guarantees

All Meyer's singletons provide:
- **Thread-safe initialization** (C++11 static local guarantee)
- **Safe concurrent access** to `getInstance()`
- **Automatic destruction** at program exit

However, the objects themselves may have their own thread-safety requirements documented in their headers.

### Static Initialization Order

Meyer's singletons **solve** the static initialization order fiasco because:
1. No static objects are created at startup
2. Initialization happens on first access (runtime)
3. Order is determined by call order, not linker order

### Testing Considerations

In unit tests:
```cpp
// Singletons persist across test runs
// For tests that need fresh state:

// Option 1: Use separate test instances (preferred)
MyManager manager;  // Stack object

// Option 2: Clean singleton state
MyManager::getInstance().reset();  // If reset() method exists

// Option 3: Accept singleton state (most common)
// Most singletons are designed to work across multiple uses
```

## Common Pitfalls

### 1. Forgetting Explicit Initialization

Some singletons require explicit setup after construction:

```cpp
// ❌ Wrong - Settings not loaded
auto& settings = Settings::getInstance();
int bufferSize = settings.bufferSize_;  // Default value, not loaded!

// ✅ Correct
auto& settings = Settings::getInstance();
settings.load();  // Explicit load required
int bufferSize = settings.bufferSize_;  // Now loaded from disk
```

### 2. UI Access Before MessageManager

```cpp
// ❌ Wrong - No MessageManager yet
RealTimeGarbageCollector::getInstance();  // Timer won't start!
juce::MessageManager::getInstance();

// ✅ Correct
juce::MessageManager::getInstance();
RealTimeGarbageCollector::getInstance();  // Timer starts properly
```

### 3. Calling Singletons From Static Destructors

```cpp
// ❌ Dangerous - Static destruction order undefined
class MyClass {
    ~MyClass() {
        // This might be called AFTER singleton is destroyed!
        ZenithLogger::getInstance().log("Destroying...");
    }
};
static MyClass globalInstance;
```

## Future Improvements

Potential enhancements to consider:

1. **Explicit Initialization API**: Optional explicit initialization for deterministic timing
2. **Initialization Verification**: Runtime checks for proper initialization order
3. **Dependency Injection**: For better testability in critical components
4. **Initialization Profiling**: Measure singleton initialization times

## References

- [C++11 Thread-Safe Static Initialization](https://en.cppreference.com/w/cpp/language/storage_duration#Static_local_variables)
- [Meyer's Singleton Pattern](https://laristra.github.io/flecsi/src/developer-guide/patterns/meyers_singleton.html)
- JUCE Documentation on thread safety

## Changelog

- **2026-01-03**: Initial documentation created during lazy initialization refactor
  - Converted `RealTimeGarbageCollector` from raw pointer to Meyer's singleton
  - Converted `GlobalUndoRedo` from static unique_ptr to Meyer's singleton
  - Cleaned up `ZenithLookAndFeel` duplicate singleton patterns
  - Documented all critical singleton initialization patterns
