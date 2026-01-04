# Build Instructions

## Prerequisites

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build \
    libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev \
    libfreetype6-dev libx11-dev libxinerama-dev libxext-dev \
    libxrandr-dev libxcursor-dev libwebkit2gtk-4.0-dev \
    libglu1-mesa-dev mesa-common-dev libxcomposite-dev
```

### macOS

```bash
xcode-select --install
brew install cmake ninja
```

### Windows

1. Install Visual Studio 2022 with "Desktop development with C++" workload
2. Install CMake 3.25+ from https://cmake.org/download/

## Building

### Standard Build (Release)

```bash
# Configure
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j$(nproc)
```

### Debug Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `ZENITH_ENABLE_SKIA` | ON | Enable Skia GPU rendering |
| `ENABLE_ONNX` | ON | Enable ONNX Runtime for AI features |
| `ZENITH_ENABLE_COLLAB` | OFF | Enable collaboration features |
| `BUILD_TESTS` | OFF | Build test executables |
| `ENABLE_IPO` | OFF | Enable link-time optimization |

Example with options:
```bash
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DENABLE_IPO=ON
```

## Running

```bash
./build/Zenith\ DAW
```

## Running Tests

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --target ZenithDAWTests
./build/ZenithDAWTests_artefacts/Release/ZenithDAWTests
```

## Troubleshooting

### "Ninja not found"
Install via package manager or use Unix Makefiles:
```bash
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
```

### JUCE/Submodule Issues
```bash
git submodule update --init --recursive
```

### Skia Build Failures
Ensure you have the graphics development libraries:
```bash
# Linux
sudo apt install libgl1-mesa-dev libvulkan-dev

# macOS - Metal is included with Xcode
```

### Missing Dependencies
Check CMake output for specific missing packages and install them via your package manager.

## Platform-Specific Notes

### Linux
- Supports ALSA, JACK, and PulseAudio
- Vulkan or OpenGL for Skia rendering

### macOS
- Requires macOS 10.15+
- Uses Core Audio and Metal

### Windows
- Requires Windows 10+
- Uses WASAPI and Direct3D 12
