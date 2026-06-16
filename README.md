# Zenith DAW(large transition in progress this md is outdated until i update it later)

A professional Digital Audio Workstation built with C++20 and JUCE, featuring AI-powered creative assistance, GPU-accelerated rendering, and comprehensive safety systems.

**Version:** 0.1.0-alpha
**Platforms:** Linux, macOS, Windows

---

## Quick Links

- **[Project Status](docs/STATUS.md)** - Current capabilities and what's working
- **[Build Instructions](docs/BUILD.md)** - How to build from source
- **[Documentation Index](docs/DOCUMENTATION_INDEX.md)** - All documentation
- **[Known Issues](docs/KNOWN_ISSUES.md)** - Bugs and limitations

---

## What Works Now

| Feature | Status |
|---------|--------|
| Audio Engine | ✅ Complete | Real-time playback, recording, mixing |
| MIDI Engine | ✅ Complete | Sequencing, piano roll, timing safety |
| VST3 Hosting | ✅ Complete | Safe scanner, timeout, crash recovery |
| Built-in Instruments | ✅ Complete | ZenithPolySynth, ZenithSampler |
| Skia GPU UI | ✅ Complete | Hardware-accelerated rendering |
| AI Assistant | ✅ Complete | Grok API integration |
| Collaboration | ✅ Complete | Full ICE/STUN/TURN (needs TURN server) |
| Stem Separation | ✅ Complete | ModelManager, auto-loader, ONNX |
| Audio Export | ✅ Complete | Non-realtime, normalization, dithering |
| Safety Systems | ✅ Complete | 12 components, 100% test coverage |

**For detailed status, see [docs/STATUS.md](docs/STATUS.md)**

---

## Quick Start

### Prerequisites

**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install build-essential cmake ninja-build \
    libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev \
    libfreetype6-dev libx11-dev libxinerama-dev libxext-dev \
    libxrandr-dev libxcursor-dev libwebkit2gtk-4.0-dev \
    libglu1-mesa-dev mesa-common-dev
```

**macOS:**
```bash
xcode-select --install
brew install cmake ninja
```

**Windows:**
- Visual Studio 2022 with "Desktop development with C++" workload
- CMake 3.25+

### Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Run

```bash
./build/Zenith\ DAW
```

---

## Project Structure

```
├── apps/desktop/Source/    # Main application source
│   ├── engine/             # Audio engine, tracks, clips
│   ├── ui/                 # User interface components
│   ├── ai/                 # AI/ML features
│   ├── dsp/                # Signal processing
│   ├── instruments/        # Built-in synths and samplers
│   ├── network/            # Collaboration and API clients
│   └── tests/              # Unit and integration tests
├── cmake/                  # CMake modules
├── docs/                   # Documentation
├── backend/                # Python collaboration server
└── Content/                # Presets, samples, fonts
```

---

## Key Features

### Professional Audio Engine
- Real-time audio processing with low latency
- Track-based project structure (audio, MIDI, instrument)
- VST3 plugin hosting for effects and instruments
- Real-time parameter automation
- Sample rate conversion and format handling

### Safety Systems ⭐
Zenith DAW includes 12 safety components to prevent crashes and data loss:
- **Atomic File Writes** - No corrupted project files
- **File Locking** - Prevent concurrent access conflicts
- **Audio Validation** - Reject malformed files before loading
- **MIDI Safety** - Message validation and timing protection
- **Thread Safety** - Lock-free queues and proper synchronization

**See [docs/SAFETY.md](docs/SAFETY.md) for details.**

### AI Integration
- Grok API client for natural language commands
- Genre detection using AI models
- Intelligent preset generation
- Real-time creative assistance

### Collaboration (Internet-Ready)
- Full ICE/STUN/TURN implementation
- Real-time project collaboration across Internet
- 100% connectivity regardless of NAT type
- Cursor tracking and edit broadcasting
- DTLS-encrypted P2P connections
- CRDT-based synchronization

### Collaboration (Internet-Ready)
- Full ICE/STUN/TURN implementation
- Real-time project collaboration across Internet
- 100% connectivity regardless of NAT type
- Cursor tracking and edit broadcasting
- DTLS-encrypted P2P connections
- CRDT-based synchronization

**Note: Requires TURN server deployment (coturn or managed service).**

---

## Documentation

### Getting Started
- [Status](docs/STATUS.md) - What works and what doesn't
- [Build Instructions](docs/BUILD.md) - Platform-specific build guides
- [Developer Guide](docs/DEVELOPER.md) - Development workflow

### Architecture
- [Architecture Overview](docs/ARCHITECTURE.md) - System design
- [Threading Model](docs/THREADING_MODEL.md) - Concurrency patterns
- [Rendering Architecture](docs/RENDERING_ARCHITECTURE.md) - Skia rendering

### Specialized Topics
- [Safety Systems](docs/SAFETY.md) - Comprehensive safety documentation
- [Collaboration](docs/COLLABORATION.md) - Real-time collaboration (LAN)
- [Known Issues](docs/KNOWN_ISSUES.md) - Bug tracking

**Full index: [docs/DOCUMENTATION_INDEX.md](docs/DOCUMENTATION_INDEX.md)**

---

## Testing

### Run Tests

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --target ZenithDAWTests
./build/ZenithDAWTests_artefacts/Release/ZenithDAWTests
```

### Test Coverage
- 25 safety component tests (all passing)
- Audio engine tests
- Project state tests
- MIDI safety tests
- Memory leak detection (LeakSanitizer enabled by default)

---

## Automation

The repository includes automated agents for:
- **Testing Agent** - Runs tests on every push/PR
- **Fuzzing Agent** - Robustness testing for DSP and MIDI
- **Security Agent** - Vulnerability scanning
- **Linting Agent** - Code quality checks
- **TriageBot** - Automatic issue/PR classification

See `agents/` directory for details.

---

## Development

### Code Style
- C++20 standard
- 4-space indentation
- `camelCase` for functions/variables
- `PascalCase` for classes
- Doxygen comments for public APIs

### Threading Rules
- **Audio Thread**: No allocations, no locks, no blocking
- **Message Thread**: All UI, ProjectState mutations
- **Background Threads**: File I/O, plugin scanning, API calls

---

## Known Limitations

- **Collaboration**: Full ICE/STUN/TURN (needs TURN server deployment)
- **Stem Separation**: ONNX Runtime not integrated
- **Audio Export**: Real-time only, no offline bounce
- **VST3 Scanner**: Crashes on some plugins

See [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md) for complete list.

---

## License

See [LICENSE](LICENSE) for details.

---

## Contributing

Contributions welcome! Please:
1. Check [docs/STATUS.md](docs/STATUS.md) for current priorities
2. Read [docs/DEVELOPER.md](docs/DEVELOPER.md) for workflow
3. Follow code style guidelines
4. Add tests for new features
5. Update documentation

---

**For the latest status, see [docs/STATUS.md](docs/STATUS.md)**
