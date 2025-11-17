# Zenith DAW - Quick Start Guide

**Pure JUCE Native Architecture**

This guide will get you up and running with Zenith DAW in under 5 minutes.

---

## ⚡ Quick Start (TL;DR)

```bash
# Linux/macOS
./build.sh
./run.sh

# Windows
build.bat
run.bat
```

---

## 📋 Prerequisites

### Required Software

| Software | Version | Download |
|----------|---------|----------|
| **CMake** | 3.22+ | https://cmake.org/download/ |
| **C++ Compiler** | C++20 | See below |

### Platform-Specific Compilers

**Windows:**
- Visual Studio 2022 (Community Edition is fine)
- Download: https://visualstudio.microsoft.com/downloads/
- Select "Desktop development with C++" workload

**macOS:**
- Xcode 13+ (includes Clang)
- Install: `xcode-select --install`

**Linux:**
- GCC 10+ or Clang 12+
- Install: `sudo apt install build-essential cmake`

### What About JUCE?

**JUCE 8.0.9 is automatically fetched by CMake** - no manual installation needed!

---

## 🚀 Installation

### Step 1: Install CMake

**Linux:**
```bash
sudo apt install cmake
```

**macOS:**
```bash
brew install cmake
```

**Windows:**
Download from https://cmake.org/download/ and run installer

### Step 2: Clone Repository

```bash
git clone <repository-url>
cd daw
```

That's it! No Qt, no Electron, no Node.js, no npm install.

---

## 🔨 Building

### Automated Build (Recommended)

**Linux/macOS:**
```bash
./build.sh          # Release build
./build.sh Debug    # Debug build
```

**Windows:**
```cmd
build.bat           # Release build
build.bat Debug     # Debug build
```

The build script will:
1. Fetch JUCE 8.0.9 automatically (first run only)
2. Configure CMake
3. Build the native application

### Manual Build

```bash
# Navigate to zenith-core
cd zenith-core

# Configure (JUCE auto-fetches)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release -j
```

---

## ▶️ Running

### Using Run Scripts

**Linux/macOS:**
```bash
./run.sh
```

**Windows:**
```cmd
run.bat
```

### Direct Execution

**Linux:**
```bash
./zenith-core/build/ZenithDAW_artefacts/Release/ZenithDAW
```

**macOS:**
```bash
open ./zenith-core/build/ZenithDAW_artefacts/Release/ZenithDAW.app
```

**Windows:**
```cmd
zenith-core\build\ZenithDAW_artefacts\Release\ZenithDAW.exe
```

---

## 🎯 First Launch

When you first run Zenith DAW:

1. **Window opens** - Modern dark JUCE UI (1400x800)
2. **Audio engine initializes** - Connects to default audio device
3. **Transport controls** - Play/Stop buttons at bottom
4. **System info** - Check console for CPU, memory, audio device info

### Test the Features

- ✅ Click **Play** button - Transport starts
- ✅ Click **Stop** button - Transport stops
- ✅ Watch **CPU meter** - Updates in real-time (top-right)
- ✅ Check **Track count** - Displays number of tracks (top-right)
- ✅ View **Audio device** - Shows current audio device (bottom-left)

---

## 🔧 Configuration

### CMake Build Options

Located in `zenith-core/CMakeLists.txt`:

```bash
# Create 8 demo tracks at startup (Debug builds only)
cmake -B build -DZENITH_ENGINE_SEED_DEBUG_TRACKS=ON
```

### Audio Device Selection

Audio device is auto-selected based on platform:
- **Windows:** WASAPI (Shared/Exclusive) or ASIO
- **macOS:** CoreAudio
- **Linux:** ALSA

---

## 📁 Project Structure

```
/daw
├── CMakeLists.txt           # Root build config (delegates to zenith-core)
├── build.sh / build.bat     # Build scripts
├── run.sh / run.bat         # Run scripts
└── zenith-core/             ⭐ MAIN PROJECT
    ├── CMakeLists.txt       # JUCE app configuration
    ├── src/
    │   ├── Main.cpp         # Application entry point (ZenithApplication)
    │   ├── MainWindow.cpp   # Main window and UI component
    │   ├── Engine.cpp       # Audio engine
    │   ├── ProjectState.cpp # ValueTree project state
    │   ├── TrackAutomationSynchronizer.cpp
    │   └── CommandAPI.cpp   # Command interface
    ├── Source/
    │   └── engine/          # Track, Clip, MixerChannel primitives
    ├── include/             # Public headers
    └── tests/               # Unit tests
```

---

## 🐛 Troubleshooting

### "CMake version too old"

**Fix:**
```bash
# Install latest CMake
sudo apt install cmake  # Linux
brew install cmake      # macOS
# Or download from: https://cmake.org/download/
```

### "C++ compiler not found"

**Windows:** Install Visual Studio 2022 with "Desktop development with C++" workload

**macOS:** Run `xcode-select --install`

**Linux:** Run `sudo apt install build-essential`

### "JUCE fetch failed"

**Fix:**
```bash
# Clear CMake cache and retry
cd zenith-core
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

### "No audio devices found"

**Windows:**
- WASAPI is built-in, should work automatically
- For pro audio, install ASIO drivers for your interface

**macOS:**
- CoreAudio is built-in, should work automatically

**Linux:**
- Install ALSA: `sudo apt install libasound2-dev`
- For JACK: `sudo apt install libjack-jackd2-dev`

### "Window is blank/black"

This is normal for the Phase 0 foundation. The window shows:
- Welcome message in the center
- Status bar at top
- Transport controls at bottom
- Audio device info

---

## 🆚 Architecture Evolution

### Electron (Removed) ❌
- Web-based UI (Chromium + React + TypeScript)
- High memory usage
- Not suitable for real-time audio

### Qt/QML (Removed) ❌
- Native UI but hybrid architecture
- Complex Qt/JUCE bridge layer
- Extra dependencies

### Pure JUCE Native (Current) ✅
- **100% C++ JUCE** - No web stack, no Qt
- **Low latency** - Direct audio device access
- **Small footprint** - No embedded browser or Qt runtime
- **Industry standard** - Same framework as major DAWs

---

## 📖 Next Steps

### Learn More

1. **[README.md](./README.md)** - Full project overview
2. **[zenith-core/README.md](./zenith-core/README.md)** - Core engine documentation
3. **JUCE Documentation** - https://docs.juce.com/

### Start Developing

**Adding UI Components:**
- Edit `zenith-core/src/MainWindow.cpp`
- Add new JUCE components to `MainComponent`
- JUCE uses immediate-mode GUI (paint + resized callbacks)

**Audio Engine:**
- Core logic in `zenith-core/src/Engine.cpp`
- Audio primitives in `zenith-core/Source/engine/`
- ValueTree state in `zenith-core/src/ProjectState.cpp`

**Testing:**
```bash
cd zenith-core/build
ctest  # Run unit tests
```

---

## 🎉 You're Ready!

You now have a professional-grade native DAW running with:
- ✅ Pure C++ JUCE native UI
- ✅ Real-time audio engine (JUCE 8.0.9)
- ✅ Cross-platform support (Windows, macOS, Linux)
- ✅ ValueTree-based project state
- ✅ Track automation system
- ✅ Zero web dependencies

**Happy developing! 🎵🚀**

---

## 💡 Development Tips

### Build Performance

- **Use Release mode** for performance testing
- **Use Debug mode** for development and debugging
- **Incremental builds** are fast (only changed files rebuild)

### IDE Setup

**CLion (Recommended for JUCE):**
1. Open `zenith-core/CMakeLists.txt` as project
2. Configure CMake
3. Build and run

**VS Code:**
1. Install CMake Tools extension
2. Install C++ extension
3. Open `zenith-core` folder
4. Select kit and build

**Visual Studio:**
1. Open folder (`zenith-core/`)
2. CMake auto-configures
3. Build and run

**Xcode (macOS):**
```bash
cd zenith-core
cmake -B build -G Xcode
open build/ZenithDAW.xcodeproj
```

### Hot Tips

- **JUCE Projucer** is NOT needed - we use CMake
- **Console output** shows useful debug info (DBG() macros)
- **Audio thread** is separate from UI thread (JUCE handles this)
- **ValueTree** is the source of truth for project state

---

**Questions?** Open an issue on GitHub or check the documentation.

**Last Updated:** 2025-11-16
**Version:** 2.0 (Pure JUCE Native)
