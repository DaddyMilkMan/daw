# 🎵 Zenith DAW

> *Professional Digital Audio Workstation built with JUCE and modern C++*

[![Build Status](https://img.shields.io/badge/build-passing-success)]()
[![License](https://img.shields.io/badge/license-Proprietary-blue)]()
[![Platform](https://img.shields.io/badge/platform-Windows-blue)]()

---

## 🚀 Quick Start

### Prerequisites
- **Visual Studio 2022** (with C++ Desktop Development)
- **CMake 3.20+**
- **vcpkg** (for Skia dependencies)
- **Windows 10/11** (x64)

### Build & Run
```bash
# Clone the repository
git clone https://github.com/yourusername/zenith-daw.git
cd zenith-daw

# Build (Release with Skia)
build.bat

# Or build without Skia
build.bat --no-skia

# Run
run.bat
```

### Development Build
```bash
# Debug build
build.bat --debug

# Quick rebuild (after code changes)
rebuild.bat

# Clean build
build.bat --clean
```

---

## 📁 Project Structure

```
zenith-daw/
├── apps/
│   └── desktop/              # Desktop application
│       ├── Source/
│       │   ├── engine/       # Audio engine, tracks, clips
│       │   ├── ui/           # User interface components
│       │   ├── instruments/  # Built-in instruments (synths, samplers)
│       │   ├── dsp/          # DSP processors (stem separation, etc.)
│       │   ├── network/      # AI integration (Grok API)
│       │   ├── commands/     # Command API for AI assistant
│       │   └── rendering/    # Skia rendering system
│       ├── Resources/        # Audio samples, presets, icons
│       └── include/          # Public headers
├── Content/                  # User content (presets, examples)
├── docs/                     # Documentation
├── scripts/                  # Build and utility scripts
└── CMakeLists.txt            # Build configuration
```

---

## 🎨 Features

### ✅ **Implemented**
- **Audio Engine**: Multi-track recording and playback
- **MIDI Support**: Piano roll editor with quantization
- **Built-in Instruments**:
  - ZenithPolySynth (subtractive synthesizer)
  - ZenithSampler (multi-sample playback)
- **Effects**: Mixer with EQ, compression, reverb
- **Automation**: Lane-based parameter automation
- **Plugin Hosting**: VST3 plugin support
- **Modern UI**: Skia-based rendering with dark theme
- **AI Assistant**: "Wingman" powered by Grok API

### 🚧 **In Development**
- Additional oscillators (Osc 2/3)
- Offline export/rendering pipeline
- Advanced stem separation
- Session view (clip launcher)

---

##⚙️ Architecture

### Audio Engine
- **Sample Rate**: 44.1kHz / 48kHz
- **Buffer Size**: 128-2048 samples (configurable)
- **Bit Depth**: 32-bit float processing
- **Latency**: <10ms (ASIO/WASAPI)

### UI Framework
- **Renderer**: Skia (hardware-accelerated) + JUCE fallback
- **Theme**: Centralized `ZenithTheme` system
- **Layout**: Responsive component-based design

### Plugin Architecture
- **Format**: VST3 (via JUCE)
- **Scanning**: Automatic on startup
- **State**: Full preset save/recall

---

## 🛠️ Development

### Building from Source

#### 1. Install Dependencies
```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat

# Install Skia
.\vcpkg install skia:x64-windows
```

#### 2. Configure CMake
```bash
cd zenith-core
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DZENITH_USE_SKIA=ON ^
    -DCMAKE_PREFIX_PATH="C:/vcpkg/installed/x64-windows"
```

#### 3. Build
```bash
cmake --build . --config Release --parallel
```

### Code Style
- **Standard**: C++17
- **Naming**: 
  - Classes: `PascalCase`
  - Functions: `camelCase`
  - Members: `camelCase_` (trailing underscore)
  - Constants: `UPPER_CASE`
- **Formatting**: 2-space indents, 100-char line limit

### Contributing
See `docs/DEVELOPER_WORKFLOW.md` for detailed contribution guidelines.

---

## 📚 Documentation

- **[Architecture Overview](docs/ARCHITECTURE.md)** - System design and component interaction
- **[Build Guide](docs/INSTALL_WINDOWS.md)** - Detailed build instructions
- **[API Documentation](docs/INSTRUMENT_COMMAND_API.md)** - Command system reference
- **[Audio Guide](docs/WINDOWS_AUDIO_APIS_GUIDE.md)** - Audio driver setup

---

## 🤖 AI Integration

Zenith DAW features "**Wingman**" - an AI assistant powered by Grok:
- Natural language commands
- Preset generation
- Mix suggestions
- Stem separation
- Voice feedback

Configure API key in `Settings → AI Integration`

---

## 📦 Binary Releases

Pre-built binaries coming soon. For now, build from source.

---

## 🐛 Known Issues

- Skia rendering may require driver updates on older GPUs
- ASIO driver required for low-latency on Windows
- Some VST3 plugins may not scan correctly (report via Issues)

---

## 📄 License

**Proprietary** - All rights reserved.  
For licensing inquiries, contact: [your-email@example.com]

---

## 🙏 Acknowledgments

- **JUCE Framework** - https://juce.com
- **Skia Graphics** - https://skia.org
- **Grok AI** - https://x.ai
- **vcpkg** - https://github.com/microsoft/vcpkg

---

## 📧 Contact

- **Issues**: [GitHub Issues](https://github.com/yourusername/zenith-daw/issues)
- **Email**: support@zenith-daw.com
- **Discord**: [Join Community](https://discord.gg/zenith-daw)

---

<div align="center">
  <strong>Made with ❤️ by the Zenith Team</strong><br>
  Polished by <em>Operation Polish Dream Team</em>
</div>
