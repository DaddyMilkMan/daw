# Zenith DAW

A professional Digital Audio Workstation built with C++20 and JUCE, featuring AI-powered creative assistance, real-time collaboration, and modern GPU-accelerated rendering.

## Status

**Version:** 0.1.0-alpha  
**Platform:** Linux, macOS, Windows  
**Build:** [![Tests](https://github.com/zenith-daw/zenith/actions/workflows/test.yml/badge.svg)](https://github.com/zenith-daw/zenith/actions/workflows/test.yml)

### What Works
- Audio engine with real-time playback and recording
- MIDI sequencing and piano roll editor
- VST3 plugin hosting
- Built-in synthesizer (ZenithPolySynth) and sampler
- Skia-based GPU-accelerated UI
- Project save/load with crash recovery
- Basic mixer with routing

### In Development
- AI creative assistant (Grok integration)
- Real-time collaboration (CRDT-based)
- Stem separation (ONNX)
- Cross-platform audio export

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
- Visual Studio 2022 with C++ workload
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

## Project Structure

```
├── apps/desktop/Source/    # Main application source
│   ├── engine/             # Audio engine, tracks, clips, project state
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

## Documentation

- [Architecture Overview](docs/ARCHITECTURE.md)
- [Build Instructions](docs/BUILD.md)
- [Developer Guide](docs/DEVELOPER.md)
- [Known Issues](docs/KNOWN_ISSUES.md)

## Testing

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --target ZenithDAWTests
./build/ZenithDAWTests_artefacts/Release/ZenithDAWTests
```

### Automated Testing with Testing Agent

The repository includes an automated **Testing Agent** that runs on every push and pull request via GitHub Actions. This agent provides comprehensive test orchestration and validation:

**Current Capabilities:**
- **Test Discovery**: Automatically discovers C++ unit tests (JUCE and Catch2 frameworks)
- **Test Execution**: Runs all discovered tests and reports results
- **CI Integration**: Seamlessly integrates with GitHub Actions workflow

**Planned Features** (stub implementations, expandable in future PRs):
- **RT-Safety Analysis**: Static analysis to detect illegal allocations, locks, or blocking calls in real-time audio threads
- **Code Coverage**: Generate and report line/branch coverage metrics
- **TODO Scanning**: Surface TODO comments in critical code paths for developer triage
- **Memory Leak Detection**: Valgrind integration for leak checking
- **Audio Quality Metrics**: THD, SNR, and frequency response validation

**Running Locally:**
```bash
python agents/TestingAgent/testing_agent.py
```

**CI Workflow:**
The Testing Agent runs automatically via `.github/workflows/agent-testing.yml` on:
- Pushes to `main`, `develop`, and `copilot/**` branches
- Pull requests targeting `main` or `develop`

See [`agents/TestingAgent/README.md`](agents/TestingAgent/README.md) for implementation details and expansion roadmap.

## License

See [LICENSE](LICENSE) for details.
