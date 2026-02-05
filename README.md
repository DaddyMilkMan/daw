# Zenith DAW

A professional "Satellite DAW" built with C++20 and JUCE, designed for speed, creativity, and brainstorming. It complements your main DAW by focusing on rapid idea generation, AI-powered assistance, and real-time collaboration.

![AGPL v3](https://img.shields.io/badge/license-AGPL%20v3-blue.svg)
![GitHub issues](https://img.shields.io/github/issues/micahcooley/daw)
![GitHub pull requests](https://img.shields.io/github/issues-pr/micahcooley/daw)

**License:** [GNU Affero General Public License v3.0](LICENSE)
**Status:** Open Source - External contributions welcome!

**Become a Contributor!**

Zenith DAW is an open-source project that welcomes external developers. Whether you're a seasoned audio engineer, C++ expert, or just passionate about digital audio workstations, we'd love your help!

## Quick Start for New Contributors

### 🚀 First-Time Setup

1. **Fork and clone the repository**
   ```bash
   git clone https://github.com/YOUR_USERNAME/daw.git
   cd daw
   ```

2. **Install prerequisites** (see [Build Instructions](#build-instructions))
   - Linux/Ubuntu: `sudo apt install build-essential cmake ninja-build libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev libfreetype6-dev libx11-dev libxinerama-dev libxext-dev libxrandr-dev libxcursor-dev libwebkit2gtk-4.0-dev libglu1-mesa-dev mesa-common-dev`
   - macOS: `xcode-select --install && brew install cmake ninja`
   - Windows: Visual Studio 2022 with C++ workload

3. **Build the project**
   ```bash
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build -j$(nproc)
   ```

4. **Run tests** (optional but recommended)
   ```bash
   cmake -S . -B build -DBUILD_TESTS=ON
   cmake --build build --target ZenithDAWTests
   ./build/ZenithDAWTests_artefacts/Release/ZenithDAWTests
   ```

5. **Start contributing!** See [Contributing Guidelines](CONTRIBUTING.md) for details.

## Project Status

**Version:** 0.1.0-alpha
**Platform:** Linux, macOS, Windows
**License:** AGPL v3
**CI/CD:** [![Tests](https://github.com/micahcooley/daw/actions/workflows/test.yml/badge.svg)](https://github.com/micahcooley/daw/actions/workflows/test.yml) [![Fuzzing](https://github.com/micahcooley/daw/actions/workflows/fuzzing-agent.yml/badge.svg)](https://github.com/micahcooley/daw/actions/workflows/fuzzing-agent.yml) [![Testing Agent](https://github.com/micahcooley/daw/actions/workflows/agent-testing.yml/badge.svg)](https://github.com/micahcooley/daw/actions/workflows/agent-testing.yml) [![Security Scan](https://github.com/micahcooley/daw/actions/workflows/security-agent.yml/badge.svg)](https://github.com/micahcooley/daw/actions/workflows/security-agent.yml) [![Linting](https://github.com/micahcooley/daw/actions/workflows/linting-agent.yml/badge.svg)](https://github.com/micahcooley/daw/actions/workflows/linting-agent.yml) [![Triage](https://github.com/micahcooley/daw/actions/workflows/triage-bot.yml/badge.svg)](https://github.com/micahcooley/daw/actions/workflows/triage-bot.yml)

### Current Focus Areas for Contributors
- 🎵 **Audio Engine:** Real-time processing, plugin hosting, MIDI support
- 🎨 **UI Framework:** Skia-based components, GPU-accelerated rendering
- 🤖 **AI Integration:** Grok API client, creative assistance features
- 🔧 **Tooling:** Build system, testing infrastructure, automation
- 📱 **Platform Support:** macOS and Windows porting

### What Works
- Audio engine with real-time playback and recording
- MIDI sequencing and piano roll editor
- VST3 plugin hosting
- Built-in synthesizer (ZenithPolySynth) and sampler
- Skia-based GPU-accelerated UI
- Project save/load with crash recovery
- Basic mixer with routing

### Core Features Implemented
- ✅ Audio engine with real-time playback and recording
- ✅ MIDI sequencing and piano roll editor
- ✅ VST3 plugin hosting
- ✅ Built-in synthesizer (ZenithPolySynth) and sampler
- ✅ Skia-based GPU-accelerated UI
- ✅ Project save/load with crash recovery
- ✅ Basic mixer with routing

### Active Development Areas
- 🚧 AI creative assistant (Grok integration)
- 🚧 Real-time collaboration (CRDT-based)
- 🚧 Stem separation (ONNX)
- 🚧 Cross-platform audio export

### Good First Issues for New Contributors
Check our [Issues](https://github.com/micahcooley/daw/issues) page for labeled "good first issue" tickets. Great areas to start:
- Bug fixes in existing components
- Documentation improvements
- Test coverage additions
- UI component polish
- Audio quality enhancements

## Build Instructions

### Prerequisites

**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install build-essential cmake ninja-build     libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev     libfreetype6-dev libx11-dev libxinerama-dev libxext-dev     libxrandr-dev libxcursor-dev libwebkit2gtk-4.0-dev     libglu1-mesa-dev mesa-common-dev
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
# Configure CMake
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build the project
cmake --build build -j$(nproc)
```

**Note:** Tests are OFF by default for faster builds. Enable them with `-DBUILD_TESTS=ON` when needed.

### Run

```bash
# Linux/macOS
./build/Zenith\ DAW

# Windows
.\build\Release\Zenith\ DAW.exe
```

### Development Build (with debug info)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build
```

## Project Structure

```
├── apps/desktop/Source/    # Desktop app shell + platform code
│   ├── ai_client/          # C++ client for Python AI service
│   ├── browser/            # Browser and marketplace integration
│   ├── platform/           # OS-specific implementations
│   └── tests/              # Unit and integration tests
├── modules/                # Reusable C++ modules
│   ├── zenith_core/         # Engine, DSP, instruments, plugins, utils
│   ├── zenith_ui/           # Skia UI components + rendering
│   ├── zenith_network/      # Collaboration and network services
│   └── zenith_commands/     # Command API for AI integration
├── services/ai/            # Python backend + agents
├── tools/agents/           # CI/build/test automation agents
├── cmake/                  # CMake modules
├── docs/                   # Documentation
└── Content/                # Presets, samples, fonts
```

## Community & Support

### Getting Help

We have several channels for community support:

- **GitHub Discussions:** [Join discussions](https://github.com/micahcooley/daw/discussions) for questions and community help
- **GitHub Issues:** [Report bugs](https://github.com/micahcooley/daw/issues) or request features
- **Developer Chat:** Join our Discord server (invite link in GitHub discussions)

### Contributing

We welcome external contributions! Please see:
- **[Contributing Guidelines](CONTRIBUTING.md)** - Detailed guide for contributors
- **[Developer Guide](docs/DEVELOPER.md)** - Technical documentation
- **[Architecture Overview](docs/ARCHITECTURE.md)** - System design

### Communication Channels

- **Discord:** Real-time chat with developers and users
- **GitHub Issues:** Bug reports and feature requests
- **GitHub Discussions:** Community Q&A and brainstorming
- **Pull Requests:** Code collaboration and review

### Community Resources

- **Code of Conduct:** [View our CoC](CONTRIBUTING.md#code-of-conduct)
- **License Information:** [AGPL v3 License](LICENSE)
- **Third-Party Licenses:** See `external/vcpkg/` for dependency licenses

## Documentation

- [Architecture Overview](docs/ARCHITECTURE.md)
- [Build Instructions](docs/BUILD.md)
- [Developer Guide](docs/DEVELOPER.md)
- [Known Issues](docs/KNOWN_ISSUES.md)
- [Development Roadmap](docs/ROADMAP.md)
- [Development Roadmap](docs/ROADMAP.md)

## Testing

### Run Tests

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

### Fuzz Testing

The FuzzingAgent provides automated robustness testing for DSP code, MIDI parsing, plugin loading, and file format parsing. It runs automatically on every push and pull request via GitHub Actions.

To run locally:
```bash
cd agents/FuzzingAgent
python fuzzing_agent.py --iterations 100
```

Options:
- `--iterations N`: Number of fuzz iterations per target (default: 100)
- `--seed N`: Random seed for reproducibility
- `--target {dsp,midi,plugin,file,all}`: Specific target to fuzz (default: all)

See [`agents/FuzzingAgent/README.md`](agents/FuzzingAgent/README.md) for more details.

### Automated Security Scanning

The SecurityAgent performs automated vulnerability scanning on every push and pull request. It checks for:
- Secret/Credential leaks (API keys, tokens)
- Insecure dependencies (Python/C++)
- Suspicious file permissions
- Binary analysis (stub)

To run locally:
```bash
cd agents/SecurityAgent
python security_agent.py --scan-type full
```

See [`agents/SecurityAgent/README.md`](agents/SecurityAgent/README.md) for details.

### Code Quality & Linting

The Linting Agent performs automated code quality checks on every push and pull request:
- Naming convention enforcement (C++ and Python)
- Style rule validation (trailing whitespace, comment spacing)
- Detection of commented-out code, magic numbers, and TODOs
- Extensible for additional linters (clang-tidy, pylint, etc.)

See [`agents/LintingAgent/README.md`](agents/LintingAgent/README.md) for details.

### Memory Leak Detection

Debug builds automatically enable AddressSanitizer and LeakSanitizer to catch memory issues:

```bash
# Build in Debug mode (sanitizers enabled by default)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build

# Run tests with leak detection
cd build
LSAN_OPTIONS=suppressions=../lsan.supp ./ZenithDAWTests

# Or use the validation script
cd ..
./test_memory_leaks.sh
```

For details on recent memory leak fixes, see [docs/MEMORY_LEAK_FIXES.md](docs/MEMORY_LEAK_FIXES.md).

## Automation

### TriageBot

This repository uses an automated triage system for issues and pull requests. When you open an issue or PR, TriageBot will:

- **Automatically classify** your submission (bug, enhancement, question, etc.)
- **Assign priority** based on severity (P0-P3)
- **Add relevant labels** for components, platforms, and categories
- **Detect duplicates** to help avoid redundant issues
- **Leave helpful comments** with classification details

This helps maintainers respond faster and ensures issues are properly categorized. The bot's classification is not final - maintainers may adjust labels as needed.

For more details, see [agents/TriageBot/README.md](agents/TriageBot/README.md).

## Backend Coordinator Agents

The `agents/` directory contains coordinator agents for build automation, testing, security, and real-time audio management. See [`agents/README.md`](agents/README.md) for details on all available agents.

## License

This project is licensed under the **GNU Affero General Public License v3.0**. See [LICENSE](LICENSE) for the full license text.

### Key License Terms

- ✅ **Commercial Use:** You can use this software in commercial products
- ✅ **Modification:** You can modify and distribute the source code
- ✅ **Sharing:** When you distribute the software (modified or original), you must provide the source code
- ✅ **Network Use:** If the software is accessed via a network, users must have access to the corresponding source code

### Component Licenses

**Core Components:**
- **Zenith DAW Core:** AGPL v3 (open source)
- **AI Client:** AGPL v3 (open source)
- **UI Framework:** AGPL v3 (open source)

**Third-Party Dependencies:**
- **JUCE Framework:** Commercial license (bundled with Zenith)
- **VST3 SDK:** MIT license (open source)
- **Skia Graphics:** BSD 3-Clause (open source)
- **ONNX Runtime:** MIT license (open source)
- **vcpkg:** MIT license (open source)

### For Commercial Users

If you need a commercial license or have questions about using Zenith DAW in a proprietary product, please contact us for custom licensing terms.

---

## How to Contribute

We welcome external contributions! Here's how you can help:

1. **Report Bugs:** Use [GitHub Issues](https://github.com/micahcooley/daw/issues)
2. **Suggest Features:** Join [GitHub Discussions](https://github.com/micahcooley/daw/discussions)
3. **Submit Code:** Follow our [Contributing Guidelines](CONTRIBUTING.md)
4. **Improve Docs:** Help us make documentation clearer

### Quick Contribution Steps

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

### Support the Project

Even if you're not a developer, you can help:

- ⭐ **Star the repository** on GitHub
- 📢 **Share the project** with audio engineers and developers
- 💬 **Answer questions** in discussions
- 🐛 **Test builds** and report issues
- 📚 **Improve documentation**

### Become a Sponsor

We offer sponsorship opportunities for individuals and companies who want to support the ongoing development of Zenith DAW. Contact us for more information.

### Thank You!

Thank you for your interest in contributing to Zenith DAW. Together, we're building a powerful, open-source digital audio workstation for everyone! 🎵
