# Zenith DAW - Quick Start Guide

**Qt/QML + JUCE Hybrid Architecture**

This guide will get you up and running with the new Qt/QML-based Zenith DAW in under 10 minutes.

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
| **Qt** | 6.5+ | https://www.qt.io/download-qt-installer |
| **CMake** | 3.22+ | https://cmake.org/download/ |
| **C++ Compiler** | C++17 | See below |

### Platform-Specific Compilers

**Windows:**
- Visual Studio 2022 (Community Edition is fine)
- Download: https://visualstudio.microsoft.com/downloads/

**macOS:**
- Xcode 14+ (includes Clang)
- Install: `xcode-select --install`

**Linux:**
- GCC 11+ or Clang 14+
- Install: `sudo apt install build-essential cmake`

### Optional (for full audio functionality)

| Software | Purpose |
|----------|---------|
| **JUCE 8.0+** | Professional audio engine |
| **ASIO Drivers** | Low-latency audio (Windows) |

---

## 🚀 Installation

### Step 1: Install Qt

#### Option A: Qt Online Installer (Recommended)

1. Download Qt installer: https://www.qt.io/download-qt-installer
2. Run installer and select:
   - Qt 6.5 or later
   - Qt Quick components
   - Your compiler kit (MSVC 2022, MinGW, Clang, GCC)

#### Option B: Package Manager

**Linux (Ubuntu/Debian):**
```bash
sudo apt install qt6-base-dev qt6-declarative-dev qt6-multimedia-dev
```

**macOS (Homebrew):**
```bash
brew install qt@6
```

**Windows:**
Use the Qt Online Installer (Option A)

### Step 2: Install CMake

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

### Step 3: Clone Repository

```bash
git clone <repository-url>
cd daw
```

### Step 4: (Optional) Add JUCE

For full audio functionality, add JUCE as a submodule:

```bash
git submodule add https://github.com/juce-framework/JUCE.git
git submodule update --init --recursive
```

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

### Manual Build

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release -j

# Run
./bin/ZenithDAW  # Linux/macOS
bin\Release\ZenithDAW.exe  # Windows
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

**Linux/macOS:**
```bash
./build/bin/ZenithDAW
```

**Windows:**
```cmd
build\bin\Release\ZenithDAW.exe
```

---

## 🎯 First Launch

When you first run Zenith DAW:

1. **UI appears** - Modern dark theme with tri-pane layout
2. **Demo tracks** - 5 demo tracks are pre-loaded
3. **Transport controls** - Top bar with play, stop, record buttons
4. **Audio settings** - Check Settings → Audio to select your audio device

### Test the UI

- ✅ Click **Play** button - Transport state changes
- ✅ Adjust **Tempo** - Changes reflect in status bar
- ✅ Right-click track - Context menu appears
- ✅ Move **Volume faders** in mixer - Smooth animations
- ✅ Press **Space** - Toggles play/pause

---

## 🔧 Configuration

### Audio Device Selection

When JUCE is integrated, you can select your audio device:

```cpp
// Will be available in Settings → Audio
audioEngine.setAudioDeviceType("ASIO");     // Windows - Professional
audioEngine.setAudioDeviceType("WASAPI");   // Windows - Modern
audioEngine.setAudioDeviceType("CoreAudio"); // macOS
audioEngine.setAudioDeviceType("ALSA");     // Linux
```

### Qt Environment Variables

If Qt is not found automatically, set:

**Linux/macOS:**
```bash
export CMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
```

**Windows:**
```cmd
set CMAKE_PREFIX_PATH=C:\Qt\6.x\msvc2022_64
```

---

## 📁 Project Structure

```
/daw
├── CMakeLists.txt           # Root build configuration
├── build.sh / build.bat     # Build scripts
├── run.sh / run.bat         # Run scripts
├── src/
│   ├── qt-qml/              # Qt/QML UI application ⭐ NEW
│   │   ├── main.cpp         # Application entry point
│   │   ├── qml/             # QML UI files
│   │   └── bridge/          # Qt/JUCE integration
│   └── juce-engine/         # JUCE audio engine ⭐ NEW
│       └── Source/          # Audio processing code
└── zenith-daw/               # Old Electron code (legacy)
```

---

## 🐛 Troubleshooting

### "Qt not found"

**Fix:**
```bash
# Set Qt path
export CMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64

# Or install Qt via package manager
sudo apt install qt6-base-dev qt6-declarative-dev  # Linux
brew install qt@6                                    # macOS
```

### "CMake version too old"

**Fix:**
```bash
# Install latest CMake
sudo apt install cmake  # Linux
brew install cmake      # macOS
# Or download from: https://cmake.org/download/
```

### "JUCE not found"

**This is OK!** JUCE is optional. The app will use placeholder audio implementation.

**To add JUCE:**
```bash
git submodule add https://github.com/juce-framework/JUCE.git
```

### "Failed to load QML file"

**Fix:**
```bash
# Clean rebuild
rm -rf build
./build.sh
```

### "No audio devices found"

**Windows:** Install ASIO drivers for your audio interface, or use WASAPI (built-in)

**macOS:** CoreAudio is built-in, should work automatically

**Linux:** Install ALSA: `sudo apt install libasound2-dev`

---

## 🆚 Old vs New Architecture

### Electron (Old) - `zenith-daw/`

❌ **Deprecated** - Web-based UI (Chromium + React + Node.js)
- Located in `/zenith-daw/`
- Not recommended for new development
- Kept for reference only

### Qt/QML + JUCE (New) - `src/`

✅ **Active Development** - Native UI + professional audio
- Located in `/src/qt-qml/` and `/src/juce-engine/`
- Modern, hardware-accelerated UI
- Professional audio performance
- Industry-standard architecture

**Use the new architecture for all development!**

---

## 📖 Next Steps

### Learn More

1. **[README.md](./README.md)** - Full project overview
2. **[QT_QML_JUCE_ARCHITECTURE.md](./docs/QT_QML_JUCE_ARCHITECTURE.md)** - Technical architecture
3. **[WINDOWS_AUDIO_APIS_GUIDE.md](./docs/WINDOWS_AUDIO_APIS_GUIDE.md)** - Windows audio setup
4. **[MIGRATION_FROM_ELECTRON.md](./docs/MIGRATION_FROM_ELECTRON.md)** - Migration guide

### Start Developing

**UI Development (QML):**
- Edit files in `src/qt-qml/qml/`
- QML supports hot-reload for rapid iteration
- See Qt Quick documentation: https://doc.qt.io/qt-6/qtquick-index.html

**Audio Development (C++):**
- Edit files in `src/juce-engine/Source/`
- Rebuild after changes: `./build.sh`
- See JUCE documentation: https://docs.juce.com/

**Qt/JUCE Bridge:**
- Edit `src/qt-qml/bridge/AudioEngineInterface.h/cpp`
- Exposes audio engine to QML via Qt properties

---

## 🎉 You're Ready!

You now have a professional-grade DAW running with:
- ✅ Modern Qt/QML UI with hardware-accelerated animations
- ✅ Professional audio engine foundation (JUCE)
- ✅ Cross-platform support (Windows, macOS, Linux)
- ✅ All Windows audio APIs (ASIO, WASAPI, DirectSound, MME)

**Happy developing! 🎵🚀**

---

## 💡 Tips

### Development Workflow

1. **Edit QML files** - UI changes
2. **Rebuild if needed** - `./build.sh`
3. **Run** - `./run.sh`
4. **Iterate** - Repeat

### Performance

- Build in **Release** mode for performance
- Build in **Debug** mode for debugging
- Use `ccache` to speed up rebuilds (Linux/macOS)

### IDE Setup

**Qt Creator (Recommended):**
1. Open `CMakeLists.txt` as project
2. Configure kit (Qt 6.5+)
3. Build and run

**VS Code:**
1. Install CMake Tools extension
2. Install Qt extension
3. Configure CMake kit
4. Build and debug

**Visual Studio:**
1. Open folder (`daw/`)
2. CMake auto-configures
3. Build and run

---

**Questions?** Open an issue on GitHub or check the documentation.

**Last Updated:** 2025-11-10
**Version:** 1.0
