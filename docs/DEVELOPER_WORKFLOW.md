# Developer Workflow — Zenith DAW

**Native Windows • No WSL • No MSYS2 • JUCE Auto-Fetched via CMake**

This guide covers the development workflow for contributing to Zenith DAW on Windows using Visual Studio 2022 and native tooling.

---

## Development Philosophy

Zenith DAW is built as a **native Windows application** using:

- **Language**: C++20
- **Framework**: JUCE 8.0.9 (auto-fetched via CMake FetchContent)
- **Build System**: CMake ≥ 3.22
- **Compiler**: MSVC v143 (Visual Studio 2022)
- **Audio APIs**: WASAPI, ASIO (via JUCE)

**We do NOT use:**
- WSL (Windows Subsystem for Linux)
- MSYS2 or MinGW
- vcpkg or Conan
- Qt (migrated away from Qt/QML to native JUCE)

---

## Project Structure

```
daw/
├── zenith-core/              # Main native JUCE application
│   ├── CMakeLists.txt        # JUCE build system (auto-fetches JUCE 8.0.9)
│   ├── src/                  # C++ source files
│   │   ├── Main.cpp          # Application entry point
│   │   ├── MainWindow.cpp    # Main window and UI setup
│   │   ├── Engine.cpp        # Audio engine coordination
│   │   └── ProjectState.cpp  # Session/project state management
│   ├── Source/engine/        # Core engine primitives
│   │   ├── Track.h/.cpp      # Track representation
│   │   ├── Clip.h/.cpp       # Audio/MIDI clips
│   │   └── MixerChannel.h/.cpp # Mixer channel strips
│   ├── include/              # Public headers
│   ├── tests/                # Unit tests
│   └── build/                # Build artifacts (gitignored)
├── docs/                     # Documentation
│   ├── INSTALL_WINDOWS.md    # Windows installation guide
│   └── DEVELOPER_WORKFLOW.md # This file
└── README.md                 # Project overview
```

---

## Daily Development Workflow

### 1. Setting Up Your Environment

Follow the [Windows Installation Guide](INSTALL_WINDOWS.md) to install:
- Visual Studio 2022 (Desktop development with C++)
- CMake ≥ 3.22
- Git

### 2. Clone and Configure

```powershell
# Clone the repository
git clone https://github.com/DaddyMilkMan/daw.git
cd daw/zenith-core

# Configure Debug build (auto-fetches JUCE on first run)
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug

# Configure Release build
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release
```

**Note**: The first CMake configure will fetch JUCE 8.0.9 from GitHub (~150MB). This is automatic and requires no manual intervention.

### 3. Open in Visual Studio 2022

**Recommended: Open Folder**

1. **File → Open → Folder**
2. Navigate to `zenith-core/`
3. Visual Studio detects `CMakeLists.txt` automatically
4. CMake integration is built-in—no extensions needed

**Alternative: Generate Solution**

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
start build/ZenithDAW.sln
```

### 4. Building

**Via Visual Studio:**
- **Build → Build All** (Ctrl+Shift+B)
- Or right-click `ZenithDAW` target → **Build**

**Via Command Line:**

```powershell
# Debug build (with symbols, slower runtime)
cmake --build build/Debug -j

# Release build (optimized, fast runtime)
cmake --build build/Release -j
```

**Incremental builds** are fast (~10-30 seconds for small changes).

### 5. Running and Debugging

**From Visual Studio:**
- **Debug → Start Debugging (F5)** — Launches with debugger attached
- **Debug → Start Without Debugging (Ctrl+F5)** — Launches standalone

**From Command Line:**

```powershell
# Run Debug build
.\build\Debug\ZenithDAW_artefacts\Debug\"Zenith DAW.exe"

# Run Release build
.\build\Release\ZenithDAW_artefacts\Release\"Zenith DAW.exe"
```

**Debugging Tips:**
- Set breakpoints in `.cpp` files
- Use **Debug → Windows → Output** for logging
- JUCE uses `DBG()` macro for debug output
- Check the JUCE console window for assertions

---

## Coding Standards

### General Guidelines

- **C++ Standard**: C++20 (`CMAKE_CXX_STANDARD 20`)
- **Naming**:
  - Classes: `PascalCase` (e.g., `MainWindow`, `TrackView`)
  - Functions/methods: `camelCase` (e.g., `updateTrackList()`)
  - Member variables: `camelCase` (e.g., `trackHeight`, `isPlaying`)
  - Constants: `UPPER_SNAKE_CASE` or `kPascalCase`
- **JUCE Conventions**:
  - Inherit from JUCE base classes (`Component`, `AudioProcessor`, etc.)
  - Use `juce::` namespace explicitly
  - Prefer `std::unique_ptr` and `std::shared_ptr` over raw pointers
  - Use `juce::String` for text, `juce::File` for filesystem

### Performance Rules

**Audio Thread Safety:**
- **NEVER allocate** in audio callbacks (`getNextAudioBlock`, `processBlock`)
- **NEVER lock mutexes** in audio thread (use lock-free structures)
- Use `juce::SpinLock` or `juce::AbstractFifo` for cross-thread communication
- See `Source/win/WinRtAudioPriority.cpp` for MMCSS thread priority setup

**UI Thread Safety:**
- **NEVER allocate** in `paint()` or `repaint()` hot paths
- Cache geometry and pre-compute layouts
- Use `MessageManager::callAsync()` for cross-thread UI updates
- Virtualize large lists (e.g., `TrackView` only renders visible tracks)

### JUCE-Specific Patterns

```cpp
// Good: JUCE smart pointers
std::unique_ptr<juce::AudioDeviceManager> deviceManager;

// Good: JUCE callbacks
void timerCallback() override {
    // Safe to call from message thread
}

// Good: Lazy initialization
juce::LazyInitialiser<ExpensiveResource> resource;

// Bad: Raw pointers (use smart pointers instead)
Component* comp = new Component();  // ❌

// Bad: Direct audio thread access (use lock-free queues)
std::vector<float> buffer;  // ❌ in audio callback
```

---

## Testing

### Running Tests

Zenith includes unit tests built with JUCE's testing framework:

```powershell
# Build tests
cmake --build build/Debug --target ProjectStateTests

# Run tests
.\build\Debug\ProjectStateTests_artefacts\Debug\ProjectStateTests.exe
```

Or use CTest:

```powershell
cd build/Debug
ctest --output-on-failure
```

### Writing Tests

Tests are located in `tests/`. Example:

```cpp
// tests/MyComponentTests.cpp
#include <JuceHeader.h>

class MyComponentTests : public juce::UnitTest {
public:
    MyComponentTests() : juce::UnitTest("MyComponent") {}

    void runTest() override {
        beginTest("Component initialization");
        MyComponent comp;
        expect(comp.isVisible() == false);
    }
};

static MyComponentTests myComponentTests;
```

---

## Git Workflow

### Branching Strategy

- **main**: Stable releases only
- **Feature branches**: `feature/your-feature-name`
- **Bug fixes**: `fix/issue-description`

### Making Changes

```powershell
# Create feature branch
git checkout -b feature/add-mixer-panel

# Make changes, test, commit
git add src/MixerPanel.cpp
git commit -m "Add mixer panel with level meters"

# Push to remote
git push origin feature/add-mixer-panel
```

### Pull Request Guidelines

- **Keep PRs small** (one feature or subsystem at a time)
- **Test thoroughly** before submitting
- **Describe changes** clearly in PR description
- **Run tests** (`ctest`) before pushing
- **Follow coding standards** (see above)

---

## CMake Build Options

Customize your build with CMake options:

```powershell
# Enable MMCSS audio thread priority (Windows default: ON)
cmake -S . -B build -DZENITH_ENABLE_MMCSS=ON

# Enable debug track seeding (creates 8 demo tracks in Debug builds)
cmake -S . -B build -DZENITH_ENGINE_SEED_DEBUG_TRACKS=ON

# Enable experimental Phase 1 audio skeleton (not ready, default: OFF)
cmake -S . -B build -DZENITH_ENABLE_PHASE1_AUDIO=OFF
```

View all options in `zenith-core/CMakeLists.txt` lines 19-25.

---

## Profiling and Performance

### Windows-Specific Tools

**Event Tracing for Windows (ETW) / Windows Performance Analyzer (WPA):**
- Capture system-wide traces (CPU, disk, audio thread timing)
- Download: [Windows Performance Toolkit](https://docs.microsoft.com/en-us/windows-hardware/test/wpt/)

**PresentMon:**
- Monitor frame pacing and render latency
- Download: [Intel PresentMon](https://github.com/GameTechDev/PresentMon)

**Visual Studio Profiler:**
- **Debug → Performance Profiler** (CPU Usage, Memory Usage, GPU Usage)
- Use **Instrumentation** for detailed call trees

### JUCE Profiling

JUCE includes built-in profiling macros:

```cpp
#include <juce_core/juce_core.h>

void expensiveOperation() {
    JUCE_SCOPED_TRACE("expensiveOperation");
    // ... code ...
}
```

Enable with `-DJUCE_ENABLE_ALLOCATION_HOOKS=1` in CMake.

---

## Audio APIs on Windows

Zenith DAW supports all major Windows audio APIs via JUCE:

| API | Latency | Best For |
|-----|---------|----------|
| **ASIO** | 1-10ms | Professional audio interfaces |
| **WASAPI Exclusive** | 10-15ms | Modern Windows (Vista+) |
| **WASAPI Shared** | 30-50ms | Background playback |
| **DirectSound** | 50-80ms | Legacy compatibility |

**Default priority**: ASIO → WASAPI → DirectSound → MME

See [WINDOWS_AUDIO_APIS_GUIDE.md](WINDOWS_AUDIO_APIS_GUIDE.md) for details.

---

## Common Tasks

### Adding a New UI Component

1. Create header/source in `src/ui/`:
   ```cpp
   // src/ui/MixerPanel.h
   #pragma once
   #include <JuceHeader.h>

   class MixerPanel : public juce::Component {
   public:
       MixerPanel();
       void paint(juce::Graphics&) override;
       void resized() override;
   };
   ```

2. Add to `CMakeLists.txt`:
   ```cmake
   target_sources(ZenithDAW PRIVATE
       src/ui/MixerPanel.cpp
   )
   ```

3. Rebuild:
   ```powershell
   cmake --build build/Debug -j
   ```

### Adding a New Engine Class

1. Create in `Source/engine/`:
   ```cpp
   // Source/engine/Effect.h
   #pragma once
   class Effect {
   public:
       virtual void process(float* buffer, int numSamples) = 0;
   };
   ```

2. Add to `CMakeLists.txt`:
   ```cmake
   target_sources(ZenithDAW PRIVATE
       Source/engine/Effect.h
       Source/engine/Effect.cpp
   )
   ```

3. Include in your audio processing chain

### Debugging Audio Issues

1. **Enable MMCSS logging**:
   - Check `Source/win/WinRtAudioPriority.cpp`
   - Look for "Pro Audio priority enabled" in debug output

2. **Check buffer underruns**:
   - Use `juce::AudioDeviceManager::addAudioCallback()`
   - Monitor `getXRunCount()` for dropouts

3. **Profile audio thread**:
   - Use ETW/WPA to capture audio thread timing
   - Look for priority inversions or excessive CPU usage

---

## Resources

- **JUCE Documentation**: https://docs.juce.com/
- **JUCE Forums**: https://forum.juce.com/
- **Windows Audio APIs**: [WINDOWS_AUDIO_APIS_GUIDE.md](WINDOWS_AUDIO_APIS_GUIDE.md)
- **Zenith README**: [README.md](../README.md)
- **Zenith Roadmap**: See README.md "Roadmap" section

---

## FAQ

**Q: Do I need WSL to develop Zenith on Windows?**
**A:** No. Zenith is a native Windows application built with MSVC. WSL is not required or recommended.

**Q: Do I need to manually install JUCE?**
**A:** No. CMake automatically fetches JUCE 8.0.9 via `FetchContent` during the configure step.

**Q: Can I use Visual Studio Code instead of Visual Studio?**
**A:** Yes, but you'll need to install MSVC separately (via Build Tools for Visual Studio 2022) and configure CMake manually. Visual Studio 2022 is recommended for the best experience.

**Q: Why is the first build so slow?**
**A:** The first build compiles JUCE (~150MB of C++ code). Subsequent builds are incremental and much faster (~10-30 seconds).

**Q: How do I switch between Debug and Release builds?**
**A:** Use separate build directories (`build/Debug`, `build/Release`) or reconfigure with `-DCMAKE_BUILD_TYPE=Debug/Release`.

**Q: Can I build for macOS or Linux?**
**A:** Yes, Zenith is designed to be cross-platform (JUCE handles platform abstraction). However, Windows is the current development focus. macOS/Linux support is planned for later phases.

---

**Last Updated:** 2025-11-17
**Maintainer:** DaddyMilkMan
