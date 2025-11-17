# Zenith DAW - Developer Workflow

**Last Updated:** 2025-11-17
**Version:** 0.1.0
**Maintainer:** Zenith DAW Team

## Table of Contents

1. [Quick Start](#quick-start)
2. [Development Environment Setup](#development-environment-setup)
3. [Building the Project](#building-the-project)
4. [Running and Testing](#running-and-testing)
5. [Development Workflow](#development-workflow)
6. [Code Style and Standards](#code-style-and-standards)
7. [Adding New Features](#adding-new-features)
8. [Debugging](#debugging)
9. [Performance Profiling](#performance-profiling)
10. [Troubleshooting](#troubleshooting)

---

## Quick Start

### For Impatient Developers

```bash
# 1. Clone the repository
git clone <repository-url>
cd daw/zenith-core

# 2. Configure and build (Debug)
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug -j

# 3. Run
./build/Debug/ZenithDAW_artefacts/Debug/ZenithDAW  # Linux/macOS
build\Debug\ZenithDAW_artefacts\Debug\ZenithDAW.exe  # Windows

# 4. Build Release (for performance testing)
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release -j
```

---

## Development Environment Setup

### Minimum Requirements

| Component | Version | Notes |
|-----------|---------|-------|
| **CMake** | 3.22+ | Build system |
| **C++ Compiler** | C++20 support | MSVC 19.3+, GCC 11+, Clang 14+ |
| **Git** | 2.0+ | Version control |
| **JUCE** | 8.0.9 | Auto-fetched by CMake |

### Platform-Specific Requirements

#### Windows

**Required:**
- **Visual Studio 2022** (Community Edition or higher)
  - Workload: "Desktop development with C++"
  - Individual components:
    - MSVC v143 (or newer)
    - Windows 10/11 SDK
    - C++ CMake tools
  - Download: https://visualstudio.microsoft.com/downloads/

**Optional (Recommended):**
- **ASIO Drivers:** For low-latency audio (professional interfaces)
- **Windows Terminal:** Better command-line experience
- **Visual Studio Code:** Lightweight editor alternative

**Audio Testing:**
- WASAPI is built into Windows 10/11 (no drivers needed)
- ASIO requires hardware-specific drivers (check manufacturer website)

---

#### macOS

**Required:**
- **Xcode 14+** (includes Clang compiler)
  - Install from App Store or:
    ```bash
    xcode-select --install
    ```
- **CMake:**
  ```bash
  brew install cmake
  ```

**Optional (Recommended):**
- **Homebrew:** Package manager for installing tools
  ```bash
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  ```
- **CLion or Visual Studio Code:** IDEs with CMake support

**Audio Testing:**
- CoreAudio is built into macOS (no drivers needed)
- Professional interfaces: Install manufacturer's drivers

---

#### Linux

**Required (Ubuntu/Debian):**
```bash
# Build tools
sudo apt update
sudo apt install build-essential cmake git

# JUCE dependencies
sudo apt install libasound2-dev libjack-jackd2-dev \
    ladspa-sdk libcurl4-openssl-dev libfreetype6-dev \
    libx11-dev libxcomposite-dev libxcursor-dev \
    libxcursor-dev libxext-dev libxinerama-dev \
    libxrandr-dev libxrender-dev libwebkit2gtk-4.0-dev \
    libglu1-mesa-dev mesa-common-dev
```

**Required (Fedora/RHEL):**
```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake git alsa-lib-devel jack-audio-connection-kit-devel \
    freetype-devel libX11-devel libXcomposite-devel libXcursor-devel \
    libXext-devel libXinerama-devel libXrandr-devel libXrender-devel \
    webkit2gtk3-devel mesa-libGL-devel
```

**Audio Testing:**
- ALSA: Built into Linux kernel
- JACK: Professional audio routing (recommended for DAW work)
  ```bash
  sudo apt install jackd2 qjackctl  # Ubuntu/Debian
  ```

---

### IDE Setup

#### Visual Studio 2022 (Windows Recommended)

1. **Open Folder:**
   - File → Open → Folder → Select `daw/zenith-core`
   - VS will auto-detect CMakeLists.txt and configure

2. **Select Build Configuration:**
   - Top toolbar: Debug/Release dropdown
   - CMake Settings: `Project → CMake Settings`

3. **Build:**
   - Build → Build All (`Ctrl+Shift+B`)

4. **Run:**
   - Select `ZenithDAW.exe` from startup item dropdown
   - Debug → Start Debugging (`F5`)

---

#### Visual Studio Code (Cross-Platform)

1. **Install Extensions:**
   - C/C++ (Microsoft)
   - CMake Tools (Microsoft)
   - CMake (twxs)

2. **Open Workspace:**
   - File → Open Folder → `daw/zenith-core`

3. **Configure CMake:**
   - `Ctrl+Shift+P` → "CMake: Configure"
   - Select compiler kit (MSVC, GCC, Clang)

4. **Build:**
   - `Ctrl+Shift+P` → "CMake: Build" or `F7`

5. **Debug:**
   - `Ctrl+Shift+P` → "CMake: Debug" or `Ctrl+F5`

**Recommended settings.json:**
```json
{
  "cmake.configureOnOpen": true,
  "cmake.buildDirectory": "${workspaceFolder}/build/${buildType}",
  "files.associations": {
    "*.h": "cpp",
    "*.cpp": "cpp"
  },
  "C_Cpp.default.cppStandard": "c++20"
}
```

---

#### CLion (JetBrains)

1. **Open Project:**
   - File → Open → Select `daw/zenith-core/CMakeLists.txt`

2. **Configure:**
   - CLion auto-configures CMake
   - Settings → Build, Execution, Deployment → CMake
   - Add Debug and Release profiles

3. **Build:**
   - Build → Build Project (`Ctrl+F9`)

4. **Run:**
   - Run → Run 'ZenithDAW' (`Shift+F10`)

---

## Building the Project

### Step-by-Step Build Instructions

#### 1. Navigate to Project Directory

```bash
cd /path/to/daw/zenith-core
```

#### 2. Configure CMake (First Time)

**Debug Build:**
```bash
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug
```

**Release Build:**
```bash
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release
```

**Custom Options:**
```bash
# Enable debug track seeding (creates 8 demo tracks at startup)
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug \
      -DZENITH_ENGINE_SEED_DEBUG_TRACKS=ON

# Specify compiler (if multiple installed)
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_COMPILER=g++-11

# Windows: Specify generator
cmake -S . -B build/Debug -G "Visual Studio 17 2022" \
      -A x64 -DCMAKE_BUILD_TYPE=Debug
```

#### 3. Build

```bash
# Build Debug (parallel, uses all CPU cores)
cmake --build build/Debug -j

# Build Release
cmake --build build/Release -j

# Build specific target
cmake --build build/Debug --target ZenithDAW -j

# Clean and rebuild
cmake --build build/Debug --clean-first -j
```

#### 4. Verify Build

```bash
# Check if executable was created
ls build/Debug/ZenithDAW_artefacts/Debug/ZenithDAW      # Linux/macOS
dir build\Debug\ZenithDAW_artefacts\Debug\ZenithDAW.exe  # Windows
```

---

### Build Configurations

#### Debug

**Purpose:** Development, debugging, testing

**Characteristics:**
- No optimizations (`-O0`)
- Debug symbols (`-g`, `/DEBUG:FASTLINK`)
- Assertions enabled (`DEBUG=1`)
- Slower runtime, faster compile

**Use Cases:**
- Daily development
- Debugging with breakpoints
- Memory leak detection
- Unit testing

---

#### Release

**Purpose:** Performance testing, profiling, distribution

**Characteristics:**
- Full optimizations (`-O3`, `/O2`)
- Debug symbols (`/DEBUG:FULL` on Windows)
- Assertions disabled (`NDEBUG=1`)
- Fast runtime, slower compile

**Use Cases:**
- Performance profiling
- Audio glitch testing
- CPU usage measurements
- Beta/release builds

---

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Release | Build configuration (Debug/Release) |
| `ZENITH_ENGINE_SEED_DEBUG_TRACKS` | OFF | Create 8 demo tracks at startup |
| `ZENITH_ENABLE_MMCSS` | ON (Windows) | Enable MMCSS "Pro Audio" priority |
| `BUILD_TESTING` | ON | Build unit tests |

**Example:**
```bash
cmake -S . -B build/Debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DZENITH_ENGINE_SEED_DEBUG_TRACKS=ON \
  -DBUILD_TESTING=ON
```

---

## Running and Testing

### Running the Application

#### From Command Line

**Linux/macOS:**
```bash
./build/Debug/ZenithDAW_artefacts/Debug/ZenithDAW
```

**Windows (PowerShell):**
```powershell
.\build\Debug\ZenithDAW_artefacts\Debug\ZenithDAW.exe
```

**Windows (CMD):**
```cmd
build\Debug\ZenithDAW_artefacts\Debug\ZenithDAW.exe
```

---

#### From IDE

- **Visual Studio:** Select `ZenithDAW.exe` and press `F5`
- **VS Code:** `Ctrl+Shift+P` → "CMake: Debug"
- **CLion:** `Shift+F10`

---

### Running Tests

**Build and Run All Tests:**
```bash
# Configure with tests enabled
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON

# Build
cmake --build build/Debug -j

# Run tests
cd build/Debug
ctest --output-on-failure
```

**Run Specific Test:**
```bash
./build/Debug/ProjectStateTests  # Linux/macOS
build\Debug\ProjectStateTests.exe  # Windows
```

**Test Output:**
```
[==========] Running 10 tests
[ RUN      ] ProjectStateTest.CreateNewProject
[       OK ] ProjectStateTest.CreateNewProject (2 ms)
[ RUN      ] ProjectStateTest.AddTrack
[       OK ] ProjectStateTest.AddTrack (1 ms)
...
[==========] 10 tests ran (15 ms total)
[  PASSED  ] 10 tests
```

---

### Manual Testing Checklist

After building, verify core functionality:

- [ ] **Launch:** Application starts without errors
- [ ] **Audio Device:** Status bar shows current audio device
- [ ] **Transport:** Play/Stop buttons toggle playback state
- [ ] **CPU Usage:** CPU label updates in real-time
- [ ] **Track Count:** Label shows "Tracks: 0" (or 8 if debug seed enabled)
- [ ] **Window Resize:** UI responds to window resize
- [ ] **Close:** Application closes cleanly (no crashes)

---

## Development Workflow

### Branch Strategy

```
main (protected)
  ↓
  └── claude/feature-name-<session-id> (feature branches)
```

**Rules:**
1. **Never commit directly to `main`**
2. **Create feature branches** with prefix `claude/`
3. **Use descriptive names:** `claude/fix-automation-bugs-01XYZ`
4. **Keep branches short-lived:** Merge within 1-2 weeks
5. **Delete after merge:** Clean up merged branches

---

### Feature Development Process

#### 1. Create Feature Branch

```bash
# Start from latest main
git checkout main
git pull origin main

# Create feature branch
git checkout -b claude/add-piano-roll-01ABC123
```

#### 2. Make Changes

**Small, Incremental Commits:**
```bash
# Make changes to files
vim src/PianoRoll.cpp

# Stage changes
git add src/PianoRoll.cpp include/PianoRoll.h

# Commit with descriptive message
git commit -m "Add basic Piano Roll component with grid rendering"
```

**Commit Message Format:**
```
<type>: <short summary>

<optional detailed description>

- Bullet points for key changes
- Reference issues if applicable
```

**Types:** `feat`, `fix`, `refactor`, `docs`, `test`, `perf`, `chore`

**Examples:**
```
feat: Add Piano Roll component with MIDI note rendering

- Implement PianoRollComponent class
- Add grid rendering with beat divisions
- Support zoom and scroll
- Add note selection and drag

Closes #42
```

```
fix: Correct automation interpolation for mute parameter

Mute automation was using linear interpolation instead of step
interpolation, causing partial mute states. Now uses step
interpolation (value changes only at exact point positions).

Fixes #56
```

#### 3. Test Locally

```bash
# Build
cmake --build build/Debug -j

# Run
./build/Debug/ZenithDAW_artefacts/Debug/ZenithDAW

# Test manually (see checklist above)

# Run unit tests
cd build/Debug && ctest --output-on-failure
```

#### 4. Push to Remote

```bash
# First push (create remote branch)
git push -u origin claude/add-piano-roll-01ABC123

# Subsequent pushes
git push
```

#### 5. Create Pull Request

1. Go to GitHub repository
2. Click "Compare & pull request"
3. Fill out PR template:
   - **Title:** Clear, concise summary
   - **Description:** What, why, how
   - **Testing:** Steps to verify
   - **Screenshots:** If UI changes

**PR Template:**
```markdown
## Summary
Add Piano Roll component with MIDI note rendering and editing.

## Changes
- Implemented `PianoRollComponent` class
- Added grid rendering with beat/bar divisions
- Implemented note selection and drag-to-move
- Added zoom and scroll support

## Testing
- [x] Manual testing: Created MIDI clip, verified note rendering
- [x] Manual testing: Verified zoom/scroll responsiveness
- [ ] Unit tests: TODO (will add in follow-up PR)

## Screenshots
![Piano Roll Screenshot](screenshot.png)

Closes #42
```

#### 6. Code Review

- Address reviewer feedback
- Make requested changes
- Push updates to same branch (PR auto-updates)

#### 7. Merge and Clean Up

```bash
# After PR approved and merged
git checkout main
git pull origin main

# Delete local feature branch
git branch -d claude/add-piano-roll-01ABC123

# Delete remote branch (if not auto-deleted)
git push origin --delete claude/add-piano-roll-01ABC123
```

---

### Daily Development Workflow

**Morning:**
1. Pull latest `main`
2. Check for breaking changes in team chat/PRs
3. Rebuild if necessary
4. Run tests to verify local environment

**During Development:**
1. Make small, focused changes
2. Build frequently (`cmake --build build/Debug -j`)
3. Test manually after each significant change
4. Commit early and often

**Before Pushing:**
1. Build Release config
2. Run full test suite
3. Verify no compiler warnings
4. Check for memory leaks (if applicable)

**End of Day:**
1. Commit all work-in-progress
2. Push to remote (backup)
3. Update task tracking (if applicable)

---

## Code Style and Standards

### C++ Coding Standards

#### Naming Conventions

```cpp
// Classes: PascalCase
class AudioEngine { };
class TrackComponent { };

// Functions/Methods: camelCase
void processAudio();
double getSampleRate() const;

// Member variables: camelCase with trailing underscore
class Engine {
private:
    double sampleRate_;
    bool isPlaying_;
};

// Constants: ALL_CAPS or kPascalCase
const int BUFFER_SIZE = 512;
const double kDefaultTempo = 120.0;

// Namespaces: lowercase
namespace zenith {
namespace audio {
```

#### File Organization

```cpp
// Header file: PianoRoll.h
#pragma once

#include <JuceHeader.h>

// Forward declarations (avoid #includes if possible)
class MidiSequence;

/**
 * @brief Piano Roll component for MIDI editing
 *
 * Detailed description...
 */
class PianoRoll : public juce::Component
{
public:
    // Public interface
    PianoRoll();
    ~PianoRoll() override;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Private helpers
    void renderGrid(juce::Graphics& g);

    // Member variables
    int gridDivision_ = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRoll)
};
```

#### Thread Safety Comments

**CRITICAL:** Mark thread affinity for all audio-related code:

```cpp
class Engine {
public:
    // MESSAGE THREAD ONLY
    void play();
    void stop();

    // AUDIO THREAD (real-time safe!)
    void audioDeviceIOCallback(...) override;

private:
    // SHARED (atomic for lock-free access)
    std::atomic<bool> isPlaying_{false};

    // MESSAGE THREAD ONLY
    juce::AudioDeviceManager deviceManager;

    // AUDIO THREAD ONLY (no message thread access!)
    double phase_ = 0.0;
};
```

---

### JUCE Conventions

#### Memory Management

```cpp
// Use std::unique_ptr for ownership
std::unique_ptr<Engine> engine;

// Use std::shared_ptr sparingly (prefer unique_ptr)
std::shared_ptr<ProjectState> state;

// JUCE components: use std::unique_ptr or addAndMakeVisible
class MainComponent : public juce::Component {
private:
    std::unique_ptr<juce::TextButton> playButton;
    // or
    juce::TextButton playButton;  // Owned by Component
};
```

#### ValueTree Patterns

```cpp
// Define identifiers as static members
static const juce::Identifier ID_TRACK("TRACK");
static const juce::Identifier PROP_NAME("name");

// Access ValueTree properties
juce::String name = track.getProperty(PROP_NAME, "Untitled");

// Modify with UndoManager
track.setProperty(PROP_NAME, "New Name", undoManager);

// Listen for changes
class MyListener : private juce::ValueTree::Listener {
    void valueTreePropertyChanged(juce::ValueTree& tree,
                                   const juce::Identifier& property) override
    {
        if (property == PROP_NAME)
            nameChanged();
    }
};
```

---

### Audio Thread Rules (Critical!)

**NEVER on audio thread:**
```cpp
// ❌ NO: Memory allocation
auto* buffer = new float[1024];
std::vector<float> data;
data.push_back(0.5f);

// ❌ NO: Mutex locks
std::lock_guard<std::mutex> lock(mutex_);
juce::ScopedLock lock(criticalSection);

// ❌ NO: System calls
DBG("Processing audio");  // Uses logging
File::loadFileAsString("project.zth");

// ❌ NO: UI updates
component->repaint();
```

**ALWAYS allowed on audio thread:**
```cpp
// ✅ YES: Read atomics
float volume = volumeAtomic_.load();

// ✅ YES: Fixed-size math
float output = input * gain;

// ✅ YES: Pre-allocated buffers
preallocatedBuffer_.copyFrom(0, 0, input, numSamples);

// ✅ YES: Lock-free queues (SPSC)
if (midiQueue_.pop(message))
    processMidiMessage(message);
```

---

## Adding New Features

### Checklist for New Features

- [ ] **Design:** Document architecture in PR description
- [ ] **Interfaces:** Define clear public API
- [ ] **Thread Safety:** Mark thread affinity in comments
- [ ] **ValueTree:** Use for persistent state
- [ ] **Undo Support:** All mutations use UndoManager
- [ ] **Error Handling:** Graceful failures, user-facing messages
- [ ] **Documentation:** Update docs/ if architecture changes
- [ ] **Testing:** Add unit tests or manual test plan
- [ ] **Performance:** Profile if audio-thread code added

---

### Adding a New UI Component

**Example: Adding a Tempo Control**

1. **Create Header:** `include/TempoControl.h`
```cpp
#pragma once
#include <JuceHeader.h>

class ProjectState;

class TempoControl : public juce::Component,
                     private juce::ValueTree::Listener
{
public:
    TempoControl(ProjectState& state);
    ~TempoControl() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void valueTreePropertyChanged(juce::ValueTree&,
                                   const juce::Identifier&) override;

    ProjectState& projectState_;
    juce::Label tempoLabel_;
    juce::TextEditor tempoEditor_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoControl)
};
```

2. **Implement:** `src/TempoControl.cpp`
```cpp
#include "../include/TempoControl.h"
#include "../include/ProjectState.h"

TempoControl::TempoControl(ProjectState& state)
    : projectState_(state)
{
    // Setup UI
    addAndMakeVisible(tempoLabel_);
    tempoLabel_.setText("Tempo:", juce::dontSendNotification);

    addAndMakeVisible(tempoEditor_);
    tempoEditor_.setText(juce::String(state.getTempo()));
    tempoEditor_.onReturnKey = [this] {
        double tempo = tempoEditor_.getText().getDoubleValue();
        projectState_.setTempo(tempo);
    };

    // Listen for external tempo changes
    projectState_.getValueTree().addListener(this);
}

void TempoControl::resized()
{
    auto bounds = getLocalBounds();
    tempoLabel_.setBounds(bounds.removeFromLeft(60));
    tempoEditor_.setBounds(bounds);
}

void TempoControl::valueTreePropertyChanged(
    juce::ValueTree& tree, const juce::Identifier& property)
{
    if (property == ProjectState::PROP_TEMPO)
        tempoEditor_.setText(juce::String(projectState_.getTempo()));
}
```

3. **Integrate:** Add to `MainComponent`
```cpp
// MainWindow.h
std::unique_ptr<TempoControl> tempoControl_;

// MainWindow.cpp
tempoControl_ = std::make_unique<TempoControl>(*projectState);
addAndMakeVisible(tempoControl_.get());
```

4. **Update CMakeLists.txt:**
```cmake
target_sources(ZenithDAW PRIVATE
    # ... existing files ...
    src/TempoControl.cpp
)
```

---

### Adding Engine Functionality

**Example: Adding Master Volume Control**

1. **Extend ProjectState:** Add master volume to ValueTree schema
2. **Add Atomic:** Add `std::atomic<float> masterVolume_` to Engine
3. **Synchronize:** Update atomic from ValueTree in message thread
4. **Apply:** Read atomic in audio callback, apply to output

See `docs/ARCHITECTURE_OVERVIEW.md` section "Adding New Features" for detailed patterns.

---

## Debugging

### Debug Builds

**Enable Debug Symbols:**
```bash
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug -j
```

**Debug Output:** JUCE `DBG()` macro prints to:
- Windows: Visual Studio Output window or DebugView
- macOS: Console.app or Xcode console
- Linux: Terminal (stdout)

---

### Debugger Setup

#### Visual Studio

1. **Set Breakpoint:** Click left margin in code editor (red dot)
2. **Start Debugging:** `F5`
3. **Step Over:** `F10`
4. **Step Into:** `F11`
5. **Continue:** `F5`

**Watch Variables:**
- Debug → Windows → Watch → Add variable name

**Call Stack:**
- Debug → Windows → Call Stack

---

#### GDB (Linux/macOS)

```bash
# Build with debug symbols
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug -j

# Launch with GDB
gdb ./build/Debug/ZenithDAW_artefacts/Debug/ZenithDAW

# Set breakpoint
(gdb) break Engine::audioDeviceIOCallback
(gdb) break MainWindow.cpp:42

# Run
(gdb) run

# When breakpoint hits:
(gdb) print isPlaying_       # Print variable
(gdb) backtrace              # Show call stack
(gdb) next                   # Step over (F10)
(gdb) step                   # Step into (F11)
(gdb) continue               # Continue (F5)
(gdb) quit                   # Exit GDB
```

---

#### LLDB (macOS)

```bash
# Build with debug symbols
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug -j

# Launch with LLDB
lldb ./build/Debug/ZenithDAW_artefacts/Debug/ZenithDAW

# Set breakpoint
(lldb) breakpoint set --name Engine::audioDeviceIOCallback
(lldb) b MainWindow.cpp:42

# Run
(lldb) run

# When breakpoint hits:
(lldb) frame variable           # Print all local variables
(lldb) p isPlaying_             # Print variable
(lldb) bt                       # Backtrace (call stack)
(lldb) next                     # Step over
(lldb) step                     # Step into
(lldb) continue                 # Continue
(lldb) quit                     # Exit LLDB
```

---

### Common Debugging Scenarios

#### Audio Glitches / Dropouts

1. **Check CPU Usage:** Status bar CPU label
2. **Audio Thread Violations:** Look for allocations/locks in callback
3. **Buffer Size:** Increase buffer size in audio settings
4. **Release Build:** Profile in Release (Debug is slower)

#### UI Not Updating

1. **Timer Running?** Check `startTimer()` called
2. **ValueTree Listener:** Verify listener registered
3. **Repaint Called?** Add `repaint()` after state change
4. **Thread Mismatch:** UI updates must be on message thread

#### Crash on Startup

1. **Check Logs:** Look for JUCE assertions or error messages
2. **Audio Device:** Try different audio device in settings
3. **Missing Files:** Check all resources loaded correctly
4. **Debugger:** Run in debugger to see crash location

---

## Performance Profiling

### CPU Usage Monitoring

**Built-in CPU Label:**
- Status bar shows real-time CPU usage
- Calculated by JUCE `AudioDeviceManager`

**Manual Profiling Points:**
```cpp
// Add timing code
auto start = juce::Time::getHighResolutionTicks();
processAudio(buffer);
auto end = juce::Time::getHighResolutionTicks();
auto ms = juce::Time::highResolutionTicksToSeconds(end - start) * 1000.0;
DBG("Audio processing took " + juce::String(ms) + " ms");
```

---

### Platform-Specific Profilers

#### Windows: Visual Studio Profiler

1. **Launch Profiler:** Debug → Performance Profiler
2. **Select:** CPU Usage
3. **Start:** Click "Start"
4. **Use App:** Trigger the code path to profile
5. **Stop:** Click "Stop Collection"
6. **Analyze:** View hot paths in flame graph

**ETW (Event Tracing for Windows):**
- Advanced: Use Windows Performance Analyzer (WPA)
- Tutorial: https://docs.microsoft.com/en-us/windows-hardware/test/wpt/

---

#### macOS: Instruments

```bash
# Build Release with symbols
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release -j

# Launch Instruments
open -a Instruments

# Select "Time Profiler"
# Click Record, use app, click Stop
# Analyze call tree and flame graph
```

---

#### Linux: perf

```bash
# Install perf
sudo apt install linux-tools-generic

# Build Release
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release -j

# Profile
sudo perf record -g ./build/Release/ZenithDAW_artefacts/Release/ZenithDAW

# Use app for 30 seconds, then quit

# View report
sudo perf report
```

---

### Audio Glitch Testing

**Stress Test:**
1. Set buffer size to 64 samples (or lower)
2. Load complex project (many tracks, plugins)
3. Play for extended period (5-10 minutes)
4. Monitor for audio dropouts (clicks, pops, silence)

**Acceptable Performance:**
- **CPU Usage:** < 50% average at 64 samples
- **Dropouts:** Zero in 10-minute test
- **Latency:** < 10ms round-trip (buffer + driver)

---

## Troubleshooting

### Build Errors

#### "JUCE not found"

**Problem:** CMake can't fetch JUCE

**Solution:**
```bash
# Check internet connection
ping github.com

# Clear CMake cache and retry
rm -rf build
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug
```

---

#### "C++20 features not supported"

**Problem:** Compiler too old

**Solution:**
```bash
# Check compiler version
g++ --version     # Need 11+
clang++ --version # Need 14+

# Ubuntu: Install newer GCC
sudo apt install g++-11
cmake -S . -B build/Debug -DCMAKE_CXX_COMPILER=g++-11
```

---

#### "Missing dependencies" (Linux)

**Problem:** System libraries not installed

**Solution:**
```bash
# Install all JUCE dependencies
sudo apt install libasound2-dev libjack-jackd2-dev \
    libfreetype6-dev libx11-dev libxcomposite-dev \
    libxcursor-dev libxext-dev libxinerama-dev \
    libxrandr-dev libxrender-dev libwebkit2gtk-4.0-dev \
    libglu1-mesa-dev mesa-common-dev
```

---

### Runtime Errors

#### "Audio device failed to open"

**Solutions:**
1. **Close other apps** using audio device
2. **Try different device** in Settings → Audio
3. **Windows:** Check if ASIO driver installed correctly
4. **Linux:** Start JACK server: `qjackctl`

---

#### "High CPU usage / audio dropouts"

**Solutions:**
1. **Increase buffer size** (Settings → Audio → 256 or 512 samples)
2. **Close background apps** (browsers, video players)
3. **Windows:** Check MMCSS enabled (see README)
4. **Profile:** Use profiler to find hot spots

---

#### "UI freezes / slow repaints"

**Solutions:**
1. **Check for blocking operations** in message thread
2. **Reduce timer frequency** (if custom timers added)
3. **Profile with debugger:** Look for slow paint() methods

---

### Clean Build

**When to clean:**
- CMake configuration changes not taking effect
- Linker errors after refactoring
- Strange build failures

**How to clean:**
```bash
# Nuclear option: delete build directory
rm -rf build
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug -j

# Softer option: Clean target
cmake --build build/Debug --target clean
cmake --build build/Debug -j
```

---

## Resources

### Documentation
- **JUCE Docs:** https://docs.juce.com/
- **JUCE Tutorials:** https://docs.juce.com/master/tutorial_tutorial_list.html
- **JUCE Forum:** https://forum.juce.com/
- **C++ Reference:** https://en.cppreference.com/

### Project Docs
- `docs/ARCHITECTURE_OVERVIEW.md`: System architecture
- `docs/Phase13_TrackAutomation_MVP_Summary.md`: Automation system
- `README.md`: Project overview

### Tools
- **CMake Docs:** https://cmake.org/documentation/
- **Git Docs:** https://git-scm.com/doc

---

## Getting Help

1. **Check Documentation:** Read this guide and ARCHITECTURE_OVERVIEW.md
2. **Search Issues:** GitHub Issues tab
3. **Ask Team:** Slack/Discord/team chat
4. **File Bug Report:** GitHub Issues with:
   - OS and version
   - CMake and compiler versions
   - Full error message
   - Steps to reproduce

---

**Document Version:** 1.0
**Last Updated:** 2025-11-17
**Maintained By:** Zenith DAW Team
