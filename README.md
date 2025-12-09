# 🎵 Zenith DAW

> *Professional Digital Audio Workstation built with JUCE, Skia, and modern C++*

[![Build Status](https://img.shields.io/badge/build-passing-success)]()
[![License](https://img.shields.io/badge/license-Proprietary-blue)]()
[![Platform](https://img.shields.io/badge/platform-Windows-blue)]()

---

## 🚀 Quick Start

### Prerequisites
- **Visual Studio 2022** (with C++ Desktop Development)
- **CMake 3.25+**
- **Node.js 18+** (for Authentication Service)
- **vcpkg** (Installed and integrated)
- **Windows 10/11** (x64)

### Build & Run (DAW)
```bash
# Clone the repository
git clone https://github.com/yourusername/zenith-daw.git
cd zenith-daw

# Build (Release with Skia - vcpkg will auto-install dependencies)
build.bat

# Run
run.bat
```

### Run Authentication Service
```bash
cd services/auth
npm install
npm run dev
```

---

## 📁 Project Structure

```
zenith-daw/
├── apps/
│   └── desktop/              # Desktop application (C++)
│       ├── Source/
│       │   ├── engine/       # Audio engine (Lifecycle, Transport, Tracks)
│       │   ├── ui/           # UI components (PianoRoll, Mixer, MainLayout)
│       │   │   └── skia/     # Custom Skia-based rendering widgets
│       │   ├── instruments/  # Built-in instruments (Synths, Samplers)
│       │   ├── dsp/          # Signal processing
│       │   ├── commands/     # Command pattern implementation
│       │   └── rendering/    # Graphics backend
│       └── include/          # Public headers
├── services/                 # Microservices
│   └── auth/                 # Authentication Service (Node.js/Express/MongoDB)
├── Content/                  # User content (presets, examples)
├── docs/                     # Documentation
├── logs/                     # Build and runtime logs
├── scripts/                  # Build and utility scripts
└── tools/                    # Python utilities
```

---

## 🎨 Features

### ✅ **Implemented**
- **Audio Engine**: Lock-free, multi-track recording, playback, and PDC (Plugin Delay Compensation).
- **Authentication**: Enterprise-grade Login (Email/Password + **Google OAuth 2.0**).
- **MIDI**: Professional Piano Roll with quantization, humanization, and step sequencer.
- **Instruments**: ZenithPolySynth, ZenithSampler.
- **Effects**: Mixer with EQ, compression, reverb, sends.
- **UI**: Hardware-accelerated Skia rendering with spring physics animations.
- **AI Assistant**: "Wingman" powered by Grok API.

### 🚧 **In Development**
- Advanced stem separation
- Session view (clip launcher)
- Cloud Project Sync

---

## ⚙️ Architecture

### Audio Engine
- **Sample Rate**: 44.1kHz / 48kHz
- **Bit Depth**: 32-bit float
- **Concurrency**: Lock-free FIFOs for UI-Audio communication.
- **PDC**: Automatic latency compensation for plugins.

### UI Framework
- **Renderer**: Skia (hardware-accelerated).
- **Theme**: Centralized `ZenithTheme` system.
- **Animation**: Physics-based spring animations.

### Backend
- **Service**: Node.js + Express.
- **DB**: MongoDB.
- **Auth**: JWT (Access + Refresh Tokens), Passport.js (Google Strategy).

---

## 🛠️ Development

### Code Style
- **Standard**: **C++20**
- **Formatting**: Allman braces, 4 spaces.
- **Conventions**: See `docs/AI_README.md` for detailed AI agent guidelines.

### Contributing
See `docs/DEVELOPER_WORKFLOW.md` for detailed contribution guidelines.

---

## 📚 Documentation

- **[AI Developer Guide](docs/AI_README.md)** - Critical guide for AI agents.
- **[Architecture Overview](docs/ARCHITECTURE.md)** - System design.
- **[Build Guide](docs/INSTALL_WINDOWS.md)** - Detailed build instructions.

---

## 📧 Contact

- **Issues**: [GitHub Issues](https://github.com/yourusername/zenith-daw/issues)
- **Email**: support@zenith-daw.com
- **Discord**: [Join Community](https://discord.gg/zenith-daw)

---

<div align="center">
  <strong>Made with ❤️ by the Zenith Team</strong><br>
  <em>Powered by Human-AI Collaboration</em>
</div>