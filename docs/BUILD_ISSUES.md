# Zenith DAW Build Issues and Solutions

This document provides solutions to common build issues encountered when building Zenith DAW from source. If you encounter an issue not covered here, please check [BUILDING.md](BUILDING.md) first, then open an issue on GitHub.

## Table of Contents

1. [General Build Issues](#general-build-issues)
2. [Platform-Specific Issues](#platform-specific-issues)
3. [Dependency Issues](#dependency-issues)
4. [Compiler and Linker Errors](#compiler-and-linker-errors)
5. [Runtime Issues](#runtime-issues)
6. [Performance Issues](#performance-issues)
7. [Docker Issues](#docker-issues)
8. [Getting Help](#getting-help)

## General Build Issues

### CMake Configuration Fails

**Problem:** `CMake Error: The following variables are used but they are set to NOTFOUND.`

**Solution:**
```bash
# Clean and reconfigure
rm -rf build/*
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release

# Try with verbose output
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=ON
```

### Build Takes Too Long

**Problem:** The build process is very slow.

**Solutions:**
1. **Use Ninja** (already configured)
2. **Increase parallel jobs:**
   ```bash
   cmake --build build -j$(nproc)  # Use all available cores
   ```
3. **Enable ccache for faster incremental builds:**
   ```bash
   sudo apt install ccache  # Linux
   brew install ccache     # macOS

   cmake .. -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
   ```

### Build Directory Issues

**Problem:** Build directory contains old artifacts causing conflicts.

**Solution:**
```bash
# Completely clean build directory
rm -rf build/*
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
```

### Memory Usage High

**Problem:** Build process uses excessive memory.

**Solutions:**
1. **Reduce parallel jobs:**
   ```bash
   cmake --build build -j4  # Use only 4 jobs instead of all cores
   ```
2. **Clean build directory:**
   ```bash
   rm -rf build/*
   ```

## Platform-Specific Issues

### Linux Issues

#### Audio Library Not Found

**Problem:** `Could NOT find ALSA`

**Solution:**
```bash
# Install ALSA development headers
sudo apt install libasound2-dev

# For JACK audio support
sudo apt install libjack-jackd2-dev
```

#### X11 Library Not Found

**Problem:** `Could NOT find X11`

**Solution:**
```bash
# Install X11 development headers
sudo apt install libx11-dev libxinerama-dev libxext-dev libxrandr-dev libxcursor-dev
```

#### Graphics Library Not Found

**Problem:** `Could NOT find OpenGL`

**Solution:**
```bash
# Install OpenGL development headers
sudo apt install libglu1-mesa-dev mesa-common-dev
```

#### Permission Denied

**Problem:** Permission denied errors when running the application.

**Solution:**
```bash
# Add user to audio group
sudo usermod -a -G audio $USER

# Re-login or run
newgrp audio
```

#### PulseAudio Issues

**Problem:** Application can't access audio.

**Solution:**
```bash
# Start PulseAudio
pulseaudio --start

# Or configure JACK as default
jackd -d alsa
```

### macOS Issues

#### Xcode Command Line Tools Not Found

**Problem:** `xcode-select: error: command line tools are not installed`

**Solution:**
```bash
# Install Xcode command line tools
xcode-select --install
```

#### Homebrew Not Found

**Problem:** Homebrew is not installed.

**Solution:**
```bash
# Install Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### Library Not Found

**Problem:** Library not found during linking.

**Solution:**
```bash
# Install missing libraries with Homebrew
brew install cmake ninja webkitgtk4gtk3
```

#### Code Signing Issues

**Problem:** Application won't run due to code signing.

**Solution:**
```bash
# Disable Gatekeeper for development builds
sudo spctl --add --label "Zenith DAW" /path/to/Zenith\ DAW
sudo spctl --enable /path/to/Zenith\ DAW
```

### Windows Issues

#### Visual Studio Not Found

**Problem:** `cl.exe not found`

**Solution:**
1. Open **x64 Native Tools Command Prompt for VS 2022**
2. Or set up environment:
   ```cmd
   "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
   ```

#### MSB8020 Warning

**Problem:** `MSB8020: The build tools for v142 (Platform Toolset v142)...`

**Solution:**
```cmd
# Install C++ workload for Visual Studio
# In Visual Studio Installer: Add "Desktop development with C++" workload
```

#### Windows SDK Not Found

**Problem:** Windows SDK not found.

**Solution:**
```cmd
# Install Windows SDK
# In Visual Studio Installer: Add "Windows 11 SDK"
```

#### Manifest Issues

**Problem:** Application manifest errors.

**Solution:**
```cmd
# Ensure application manifest is properly configured
# Check if resource files are included in the project
```

## Dependency Issues

### JUCE Not Found

**Problem:** `Could NOT find JUCE`

**Solutions:**

1. **Local JUCE submodule:**
   ```bash
   git submodule update --init --recursive external/JUCE
   ```

2. **System JUCE:**
   ```bash
   # Ubuntu/Debian
   sudo apt install libjuce-dev

   # macOS
   brew install juce

   # Windows
   # Download JUCE and extract to external/JUCE
   ```

3. **Fetch automatically:**
   ```bash
   # The build system should fetch JUCE automatically
   # If not, check cmake/FetchContentVersions.cmake
   ```

### ONNX Runtime Not Found

**Problem:** `Could NOT find ONNX Runtime`

**Solution:**
```bash
# Linux
sudo apt install libonnxruntime-dev

# Or build from source
git clone https://github.com/microsoft/onnxruntime.git
cd onnxruntime
./build.sh --config Release --build_wheel --enable_training --parallel
```

### OpenSSL Not Found

**Problem:** `Could NOT find OpenSSL`

**Solution:**
```bash
# Ubuntu/Debian
sudo apt install libssl-dev

# macOS
brew install openssl

# Windows
# Install vcpkg and run: vcpkg install openssl
```

### Version Conflicts

**Problem:** Dependency version conflicts.

**Solution:**
```bash
# Use CMake's find_package with version requirements
cmake .. -DOpenSSL_VERSION=3.2.0
```

## Compiler and Linker Errors

### C++20 Feature Not Supported

**Problem:** C++20 features not recognized.

**Solution:**
1. **Update compiler:**
   ```bash
   # Ubuntu: Update GCC
   sudo apt upgrade gcc g++

   # macOS: Update Xcode
   # Download latest Xcode from App Store

   # Windows: Update Visual Studio
   ```

2. **Check CMake version:**
   ```bash
   cmake --version
   # Requires CMake 3.25+
   ```

### Undefined Reference to JUCE Functions

**Problem:** Linker errors related to JUCE functions.

**Solution:**
```bash
# Clean and rebuild
rm -rf build/*
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

### Missing Standard Library Headers

**Problem:** `<filesystem>`, `<span>` headers not found.

**Solution:**
```bash
# Use C++20 standard
cmake .. -DCMAKE_CXX_STANDARD=20 -DCMAKE_CXX_STANDARD_REQUIRED=ON
```

### Link Order Issues

**Problem:** `undefined reference` errors.

**Solution:**
```bash
# Check library order in target_link_libraries
target_link_libraries(target PRIVATE
    library1
    library2
    juce_modules
)
```

### Multiple Definition Errors

**Problem:** `multiple definition of` errors.

**Solution:**
```bash
# Check for duplicate symbols
# Ensure header files have include guards
# Use `#pragma once` or `#ifndef` guards
```

## Runtime Issues

### Application Crashes on Startup

**Problem:** Application crashes immediately when launched.

**Solutions:**
1. **Run in debug mode:**
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   cmake --build .
   gdb ./build/ZenithDAW
   ```

2. **Check dependencies:**
   ```bash
   # Linux
   ldd ./build/ZenithDAW

   # macOS
   otool -L ./build/ZenithDAW

   # Windows
   depends.exe ./build/ZenithDAW.exe
   ```

3. **Check logs:**
   ```bash
   # Create log file
   ./build/ZenithDAW > log.txt 2>&1
   cat log.txt
   ```

### Audio Device Not Found

**Problem:** No audio devices available.

**Solutions:**
1. **Check audio setup:**
   ```bash
   # Linux
   aplay -l

   # macOS
   system_profiler SPAudioDataType

   # Windows
   sounddevice.query_devices()
   ```

2. **Start audio server:**
   ```bash
   # Linux with JACK
   jackd -d alsa

   # Linux with PulseAudio
   pulseaudio --start

   # macOS with Core Audio
   # Usually no additional setup needed
   ```

### Plugin Loading Issues

**Problem:** VST3 plugins not loading.

**Solutions:**
1. **Check plugin paths:**
   ```bash
   # Linux
   export VST_PATH="/path/to/plugins"

   # macOS
   export VST_PATH="/Library/Audio/Plug-ins/VST"
   export VST3_PATH="/Library/Audio/Plug-ins/VST3"

   # Windows
   set VST_PATH="C:\Program Files\Common Files\VST2"
   set VST3_PATH="C:\Program Files\Common Files\VST3"
   ```

2. **Validate plugins:**
   ```bash
   # Use plugin validation tools
   # VST3 Host or similar plugin loaders
   ```

### GUI Not Displaying

**Problem:** Application runs but no GUI appears.

**Solutions:**
1. **Check display server:**
   ```bash
   # Linux
   echo $DISPLAY
   # Should be :0 or similar

   # Check X11 connection
   xset q
   ```

2. **Install required libraries:**
   ```bash
   # Linux
   sudo apt install libx11-dev libxcursor-dev libxinerama-dev
   ```

## Performance Issues

### Slow Startup

**Problem:** Application takes a long time to start.

**Solutions:**
1. **Disable plugins during startup:**
   ```bash
   # Run with minimal plugins
   ./build/ZenithDAW --no-plugins
   ```

2. **Check audio buffer size:**
   ```bash
   # Reduce buffer size for faster startup
   ./build/ZenithDAW --buffer-size 256
   ```

### Audio Dropouts

**Problem:** Audio crackles or drops out.

**Solutions:**
1. **Reduce buffer size:**
   ```bash
   ./build/ZenithDAW --buffer-size 128
   ```

2. **Increase sample rate:**
   ```bash
   ./build/ZenithDAW --sample-rate 48000
   ```

3. **Use real-time priority:**
   ```bash
   # Linux
   sudo chrt -f 99 ./build/ZenithDAW
   ```

### High CPU Usage

**Problem:** Application uses excessive CPU.

**Solutions:**
1. **Enable performance mode:**
   ```bash
   # Linux
   sudo cpupower frequency-set -g performance

   # macOS
   sudo powermetrics --samplers cpu_power -i 1000
   ```

2. **Reduce oversampling:**
   ```bash
   ./build/ZenithDAW --oversample 1
   ```

## Docker Issues

### Build Fails in Container

**Problem:** Docker build fails with permission errors.

**Solutions:**
1. **Check Docker permissions:**
   ```bash
   # Add user to docker group
   sudo usermod -aG docker $USER
   newgrp docker
   ```

2. **Use bind mounts correctly:**
   ```bash
   docker run -v $(pwd):/workspace -w /workspace zenith-daw-builder
   ```

### Performance Issues

**Problem:** Docker builds are slow.

**Solutions:**
1. **Use SSD storage for Docker:**
   ```bash
   # Configure Docker to use SSD
   sudo systemctl edit docker
   ```

2. **Enable Docker Build Cache:**
   ```bash
   docker build --cache-from zenith-daw-builder .
   ```

### Audio Issues

**Problem:** No audio in Docker container.

**Solutions:**
1. **Forward audio devices:**
   ```bash
   # Linux with PulseAudio
   docker run -e PULSE_SERVER=host.docker.internal:4713 \
     -v /run/user/$(id -u)/pulse:/run/pulse \
     zenith-daw-builder
   ```

2. **Use JACK in container:**
   ```bash
   docker run -v /dev/snd:/dev/snd zenith-daw-builder
   ```

## Getting Help

### Before Asking for Help

1. **Read this document thoroughly**
2. **Check GitHub issues** for similar problems
3. **Update all dependencies and try again**
4. **Provide detailed error information**

### What to Include in a Bug Report

When reporting a build issue, please include:

1. **System Information:**
   - Operating system (Ubuntu 22.04, macOS 14.0, Windows 11)
   - Compiler version (GCC 12.2, Clang 14.0, MSVC 19.35)
   - CMake version (3.25.1)

2. **Error Messages:**
   - Full error output
   - Stack traces (if any)
   - Log files

3. **Steps to Reproduce:**
   - Exact commands run
   - Order of operations
   - What you expected to happen

4. **Workarounds Tried:**
   - Solutions you've already attempted

### Useful Commands for Debugging

```bash
# System information
uname -a
gcc --version
clang --version
cmake --version

# Build information
cmake --build build --target help
cmake --build build --target ZenithDAW --verbose

# Audio information
aplay -l  # Linux
system_profiler SPAudioDataType  # macOS
```

### Where to Get Help

1. **GitHub Issues:** [Report bugs and request features](https://github.com/zenith-daw/zenith/issues)
2. **GitHub Discussions:** [Ask questions and get help](https://github.com/zenith-daw/zenith/discussions)
3. **Discord:** Join our community server (link in README)
4. **Email:** support@zenithdaw.org (for private issues)

### Creating a Minimal Reproducible Example

If you're reporting a complex issue, please provide:

```bash
# Minimal CMakeLists.txt
cmake_minimum_required(VERSION 3.25)
project(Test)
add_executable(test test.cpp)
```

```cpp
// test.cpp
#include <juce_core/juce_core.h>
int main() { return 0; }
```

This helps us reproduce and fix your issue faster.

## Common Build Fixes Checklist

- [ ] Clean build directory (`rm -rf build/*`)
- [ ] Update dependencies (`git submodule update --init --recursive`)
- [ ] Check CMake version (3.25+)
- [ ] Verify compiler (GCC 12+, Clang 14+, MSVC 19.35+)
- [ ] Install system packages (Linux/macOS)
- [ ] Check Visual Studio installation (Windows)
- [ ] Verify audio device permissions
- [ ] Try different build types (Debug/Release)
- [ ] Enable verbose output (`-DCMAKE_VERBOSE_MAKEFILE=ON`)

---

Remember: Build issues are common, especially with complex projects like a DAW. Most issues have been encountered and solved before. Be patient and systematic in your troubleshooting! 🎵