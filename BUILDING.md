# Building Zenith DAW

This comprehensive guide will help you build Zenith DAW from source on Linux, macOS, and Windows. We've designed the build process to be as straightforward as possible for external contributors.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Prerequisites](#prerequisites)
3. [Build Instructions](#build-instructions)
4. [Platform-Specific Guides](#platform-specific-guides)
5. [Build Options](#build-options)
6. [Testing](#testing)
7. [Troubleshooting](#troubleshooting)
8. [Development Builds](#development-builds)
9. [Containerized Builds](#containerized-builds)
10. [Frequently Asked Questions](#frequently-asked-questions)

## Quick Start

For experienced developers, here's the fastest path to building Zenith DAW:

```bash
# Clone the repository
git clone https://github.com/zenith-daw/zenith.git
cd zenith

# Check prerequisites
./scripts/check-prerequisites.sh

# Configure and build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run the application
./build/Zenith\ DAW
```

## Prerequisites

### System Requirements

- **RAM:** 8GB minimum (16GB recommended)
- **Storage:** 4GB+ free space (builds + dependencies)
- **Compiler:** C++20 compliant compiler (GCC 12+, Clang 14+, MSVC 19.35+)
- **Build System:** CMake 3.25+

### Build Tools

All platforms require:
- **CMake 3.25+** - Build system
- **Ninja** - Build generator (faster than make)

#### Linux (Ubuntu/Debian)

```bash
# Update package lists
sudo apt update

# Install build essentials
sudo apt install build-essential cmake ninja-build git

# Audio development libraries
sudo apt install libasound2-dev libjack-jackd2-dev

# Graphics and UI libraries
sudo apt install libx11-dev libxinerama-dev libxext-dev \
                libxrandr-dev libxcursor-dev libwebkit2gtk-4.0-dev \
                libglu1-mesa-dev mesa-common-dev libfreetype6-dev

# Network and cryptography
sudo apt install libcurl4-openssl-dev libssl-dev
```

#### macOS

```bash
# Install Xcode command line tools
xcode-select --install

# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies via Homebrew
brew install cmake ninja
```

#### Windows

- **Visual Studio 2022** with "Desktop development with C++" workload
- **CMake for Windows** 3.25+
- **Ninja for Windows**

1. Download and install [Visual Studio 2022](https://visualstudio.microsoft.com/vs/2022/)
2. During installation, select "Desktop development with C++"
3. Download [Ninja](https://github.com/ninja-build/ninja/releases) and add to PATH
4. Download [CMake](https://cmake.org/download/) and install

### Optional Dependencies

These are automatically downloaded if needed, but installing system versions improves build times:

- **OpenSSL 3.0+** - For secure communications
- **libopus** - For real-time collaboration
- **ONNX Runtime** - For AI features (Linux only currently)

## Build Instructions

### Step 1: Clone the Repository

```bash
# Fork and clone from GitHub
git clone https://github.com/YOUR_USERNAME/zenith-daw.git
cd zenith-daw

# Or clone the main repository
git clone https://github.com/zenith-daw/zenith.git
cd zenith
```

### Step 2: Check Prerequisites

Use our automated prerequisites checker:

```bash
# Linux/macOS
chmod +x scripts/check-prerequisites.sh
./scripts/check-prerequisites.sh

# Windows (PowerShell)
.\scripts\check-prerequisites.ps1
```

### Step 3: Configure CMake

```bash
# Create build directory
mkdir build && cd build

# Configure CMake
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
```

Common CMake options:
- `-DCMAKE_BUILD_TYPE=Release` - Optimized build (default)
- `-DCMAKE_BUILD_TYPE=Debug` - Debug build with symbols
- `-DBUILD_TESTS=ON` - Enable tests
- `-DZENITH_ENABLE_SKIA=ON` - Enable Skia rendering (default)
- `-DZENITH_ENABLE_COVERAGE=OFF` - Disable code coverage
- `-DZENITH_ENABLE_WCET=ON` - Enable WCET monitoring (default)

### Step 4: Build the Project

```bash
# Build with all available cores
cmake --build . -j$(nproc)  # Linux/macOS
cmake --build . --config Release  # Windows
```

### Step 5: Run the Application

```bash
# Linux/macOS
./Zenith\ DAW

# Windows
.\Release\Zenith\ DAW.exe
```

## Platform-Specific Guides

### Linux Build Guide

#### Ubuntu 22.04/24.04

```bash
# Full installation script
sudo apt update
sudo apt install -y build-essential cmake ninja-build git \
                   libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev \
                   libfreetype6-dev libx11-dev libxinerama-dev libxext-dev \
                   libxrandr-dev libxcursor-dev libwebkit2gtk-4.0-dev \
                   libglu1-mesa-dev mesa-common-dev libssl-dev

# Clone and build
git clone https://github.com/zenith-daw/zenith.git
cd zenith
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

#### Audio Setup

Jack Audio Connection Kit is recommended for low-latency audio:

```bash
# Install Jack
sudo apt install libjack-jackd2-dev

# Start Jack (choose your preferred method)
# Option 1: System service
sudo systemctl --user start jack

# Option 2: With PipeWire (modern alternative)
pipewire-jack
```

### macOS Build Guide

#### Homebrew Installation

```bash
# Install Homebrew if not present
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Update Homebrew
brew update
```

#### Build Process

```bash
# Install dependencies
brew install cmake ninja

# Clone and build
git clone https://github.com/zenith-daw/zenith.git
cd zenith
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(sysctl -n hw.ncpu)
```

#### Code Signing (for running locally)

No code signing required for local development builds.

### Windows Build Guide

#### Visual Studio Setup

1. Install Visual Studio 2022
2. Select "Desktop development with C++" workload
3. Install Windows 11 SDK (included with VS)

#### Build Process

```cmd
# Create build directory
mkdir build
cd build

# Configure with CMake GUI or command line
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release

# Build with Visual Studio
cmake --build . --config Release -- /m
```

Alternatively, use the Ninja generator:

```cmd
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

#### Windows Audio Setup

Install ASIO4ALL for better audio performance:
1. Download [ASIO4ALL](https://www.asio4all.org/)
2. Install and configure in DAW preferences

## Build Options

### Core Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `ZENITH_BUILD_APP` | `ON` | Build the main application |
| `ZENITH_BUILD_TESTS` | `OFF` | Build test suite |
| `ZENITH_ENABLE_SKIA` | `ON` | Enable Skia GPU rendering |
| `ZENITH_ENABLE_COVERAGE` | `OFF` | Enable code coverage reports |
| `ZENITH_ENABLE_WCET` | `ON` | Enable worst-case execution time monitoring |

### Advanced Options

```bash
# Custom build directory
cmake .. -B custom-build

# Enable verbose output
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON

# Specific build type
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Disable dependency fetching (use system packages)
cmake .. -DZENITH_FETCH_DEPS=OFF
```

## Testing

### Enable Tests

```bash
cmake .. -DBUILD_TESTS=ON
cmake --build .
ctest --output-on-failure
```

### Run Specific Tests

```bash
# Run all tests
cmake --build . --target ZenithDAWTests

# Run specific test executable
./build/ZenithDAWTests_artefacts/Release/ZenithDAWTests
```

### Test Categories

The project uses multiple testing frameworks:
- **JUCE Unit Tests** - Audio engine and component tests
- **Catch2 Tests** - Core functionality tests
- **Integration Tests** - End-to-end workflow tests

## Troubleshooting

### Common Build Issues

#### CMake Version Too Old

**Error:** `CMake Error: CMake 3.25 or higher is required`

**Solution:**
```bash
# Ubuntu
sudo apt upgrade cmake

# macOS
brew upgrade cmake

# Windows
# Download and install latest CMake from cmake.org
```

#### Missing Dependencies

**Error:** `Could NOT find JUCE`

**Solution:** The project fetches JUCE automatically, but for faster builds:
```bash
# Clone JUCE submodule
git submodule update --init --recursive external/JUCE
```

#### Compiler Errors

**Error:** C++20 feature not supported

**Solution:**
- Update compiler: GCC 12+, Clang 14+, MSVC 19.35+
- Check compiler version:
  ```bash
  # Linux/macOS
  g++ --version
  clang++ --version

  # Windows
  cl.exe
  ```

#### Linker Errors

**Error:** Undefined reference to JUCE functions

**Solution:**
```bash
# Clean and rebuild
rm -rf build/*
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

### Platform-Specific Issues

#### Linux Audio Permissions

**Error:** `ALSA: couldn't open device`

**Solution:**
```bash
# Add user to audio group
sudo usermod -a -G audio $USER

# Re-login or run
newgrp audio
```

#### macOS Notarization

For distributed builds:
```bash
# Codesign the application
codesign --deep --force --verify --verbose --sign "Apple Developer ID: YOUR_ID" build/Zenith\ DAW
```

#### Windows Visual Studio Path Issues

**Error:** `cl.exe not found`

**Solution:**
1. Open "x64 Native Tools Command Prompt"
2. Or set up environment:
   ```cmd
   "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
   ```

### Performance Optimization

#### Build Speed

1. **Use Ninja** (already configured)
2. **Parallel builds** with `-j$(nproc)`
3. **CCache** for faster incremental builds:
   ```bash
   # Install ccache
   sudo apt install ccache  # Linux
   brew install ccache     # macOS

   # Configure CMake
   cmake .. -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
   ```

#### Memory Usage

Large builds may need increased memory:
```bash
# Monitor memory usage
htop  # Linux
top   # macOS

# Reduce parallel jobs if needed
cmake --build . -j4  # Use only 4 jobs
```

## Development Builds

### Debug Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build .
```

Debug builds include:
- Debug symbols
- AddressSanitizer for memory errors
- LeakSanitizer for memory leaks
- Debug logging

### RelWithDebInfo Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

Optimized build with debug symbols for production debugging.

### ASan Builds (Memory Debugging)

```bash
# Enable sanitizers
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DCMAKE_CXX_FLAGS="-fsanitize=address,leak -g" \
         -DCMAKE_C_FLAGS="-fsanitize=address,leak -g"

# Run tests with leak detection
LSAN_OPTIONS=suppressions=../lsan.supp ./ZenithDAWTests
```

## Containerized Builds

For consistent builds across all environments, we provide Docker support.

### Docker Build

```bash
# Build the Docker image
docker build -t zenith-daw-builder -f Dockerfile .

# Run a build container
docker run -it --rm -v $(pwd):/workspace zenith-daw-builder

# Or use docker-compose
docker-compose up build
```

### Docker Compose

```yaml
# docker-compose.yml
version: '3.8'
services:
  build:
    build:
      context: .
      dockerfile: Dockerfile
    volumes:
      - .:/workspace
    working_dir: /workspace
    command: cmake . && cmake --build . -j$(nproc)
```

### Development Container

For VS Code development:

```bash
# .devcontainer/devcontainer.json
{
  "name": "Zenith DAW Development",
  "dockerfile": "Dockerfile",
  "mounts": ["source=${localWorkspaceFolder},target=/workspace,type=bind"],
  "settings": {
    "cmake.configureOnOpen": false,
    "terminal.integrated.shell.linux": "/bin/bash"
  }
}
```

## Frequently Asked Questions

### Q: How long does the build take?

A: On a modern machine:
- **First build:** 10-20 minutes
- **Incremental builds:** 30 seconds - 2 minutes

### Q: Can I use a different build system?

A: Currently, we require CMake with Ninja. Other generators may work but aren't tested.

### Q: What if I encounter build errors?

A: See our [Troubleshooting](#troubleshooting) section. If the issue persists, check [GitHub Issues](https://github.com/zenith-daw/zenith/issues) or create a new issue.

### Q: Can I build without Skia?

A: Yes, but not recommended. Disable with `-DZENITH_ENABLE_SKIA=ON`. The UI will be less performant.

### Q: How do I update dependencies?

A: Dependencies are pinned for reproducibility. To update:
1. Edit `cmake/FetchContentVersions.cmake`
2. Run `cmake --fresh`
3. Test thoroughly before committing

### Q: What's the best IDE for development?

A:
- **Linux:** CLion, VS Code + CMake tools
- **macOS:** Xcode, CLion, VS Code
- **Windows:** Visual Studio 2022, VS Code

### Q: How do I contribute changes?

A: See our [Contributing Guidelines](CONTRIBUTING.md). The project follows standard GitHub workflow with pull requests.

### Q: Is commercial use allowed?

A: Yes, under AGPL v3. See [LICENSE](LICENSE) for details. Contact us for commercial licensing options.

## Getting Help

If you encounter issues not covered in this guide:

1. **Search existing issues:** [GitHub Issues](https://github.com/zenith-daw/zenith/issues)
2. **Ask for help:** [GitHub Discussions](https://github.com/zenith-daw/zenith/discussions)
3. **Report bugs:** Create a detailed issue with:
   - Operating system and version
   - Compiler and version
   - CMake version
   - Full error message
   - Steps to reproduce

## Contributing to Build Instructions

If you find ways to improve the build process:

1. Fork the repository
2. Make improvements to this file
3. Submit a pull request
4. Include your system information in the description

We appreciate contributions that make Zenith DAW easier to build and contribute to!