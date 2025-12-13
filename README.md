# Zenith DAW

> Early-stage Digital Audio Workstation prototype built with JUCE and modern C++

**Version:** 0.1.0 - Alpha (Not Production Ready)  
**Status:** Active Development  
**Platform:** Windows 10/11 (x64)

---

## ⚠️ Project Status

This is an **early prototype** in active development. Core features are still being implemented and stabilized. Expect bugs, incomplete features, and breaking changes.

**What works:**
- Basic audio playback engine
- MIDI input and piano roll editing
- VST3 plugin loading
- Simple synth and sampler instruments

**What doesn't work yet:**
- Reliable multi-track recording
- Project save/load (unstable)
- Plugin automation
- Export/rendering pipeline
- Most "AI features" (experimental only)

---

## 🚀 Building from Source

### Prerequisites
- Visual Studio 2022 with C++ Desktop Development
- CMake 3.25+
- vcpkg (for Skia dependencies)
- Windows 10/11 x64

### Build Steps

```bash
# 1. Install vcpkg if you haven't
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat

# 2. Install Skia
.\vcpkg install skia:x64-windows

# 3. Clone and build
git clone [your-repo-url] C:\zenith
cd C:\zenith\daw

# 4. Build (creates build directory)
.\build.bat

# 5. Run
.\run.bat
```

**Build issues?** Check `docs/INSTALL_WINDOWS.md` for troubleshooting.

---

## 📁 Project Structure

```
zenith-daw/
├── apps/desktop/
│   ├── Source/
│   │   ├── engine/        # Audio engine (tracks, clips, mixer)
│   │   ├── ui/            # UI components
│   │   ├── instruments/   # Built-in synth/sampler
│   │   └── network/       # Experimental AI integration
│   └── Resources/         # Audio samples and assets
├── docs/                  # Technical documentation
├── planning/              # Design docs and roadmaps
└── CMakeLists.txt
```

---

## 🎯 Current Development Focus

**Phase 1: Core Stability** (Current)
- Fix build system reliability
- Stabilize audio engine threading
- Implement proper error handling
- Clean up debug logging

**Phase 2: Essential Features** (Next)
- Project save/load (robust)
- Multi-track recording
- Audio export
- Plugin state management

**Phase 3: Polish** (Future)
- UI refinements
- Performance optimization
- Documentation
- Test coverage

See `planning/roadmaps/` for detailed plans.

---

## 🛠️ Development

### Code Style
- **C++ Standard:** C++17
- **Naming:** PascalCase (classes), camelCase (functions), camelCase_ (members)
- **Formatting:** 2-space indents, 100-char lines

### Key Dependencies
- **JUCE 8.0.0** - Audio framework
- **Skia** - Hardware-accelerated rendering
- **vcpkg** - Package management

### Debugging
```bash
# Debug build
.\build.bat --debug

# View logs
tail -f debug_log.txt
```

---

## 📚 Documentation

- **[Build Guide](docs/INSTALL_WINDOWS.md)** - Detailed setup instructions
- **[Architecture](docs/ARCHITECTURE.md)** - System design overview
- **[Planning Docs](planning/README.md)** - Vision and roadmaps

---

## 🤖 AI Integration (Experimental)

Some experimental AI features are in development:
- Voice command interface (via Grok API)
- Preset suggestion system

**Note:** These features are unstable and require API keys. Not recommended for testing yet.

---

## 📄 License

Proprietary - Personal/Educational use only.  
Commercial use prohibited without license.

---

## 🙏 Credits

Built with:
- [JUCE Framework](https://juce.com)
- [Skia Graphics Engine](https://skia.org)
- [vcpkg Package Manager](https://github.com/microsoft/vcpkg)

---

## 📝 Notes

This is a learning project exploring DAW architecture and audio programming. It's not intended to compete with commercial DAWs.

**Questions?** Open an issue or check the docs folder.
