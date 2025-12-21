# Zenith DAW - Build Instructions

**Last Updated**: 2025-11-27
**Platforms**: Windows (Visual Studio 2026), macOS, Linux
**Graphics Backend**: Skia (OpenGL)

---

## Prerequisites

### 1. Install Build Tools

#### Windows
- **Visual Studio 2026** (or Visual Studio 2022)
  - Download: https://visualstudio.microsoft.com/downloads/
  - Required workloads:
    - Desktop development with C++
    - Windows 10/11 SDK
  - Installation path: `C:\Program Files\Microsoft Visual Studio\18\Community\` (VS 2026)
    or `C:\Program Files\Microsoft Visual Studio\2022\Community\` (VS 2022)

- **CMake 3.20+**
  - Download: https://cmake.org/download/
  - Add to PATH during installation
  - Verify: `cmake --version`

- **Ninja Build System** (Recommended)
  - Download: https://github.com/ninja-build/ninja/releases
  - Extract to `C:\ninja\` and add to PATH
  - Verify: `ninja --version`

#### macOS
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install CMake and Ninja
brew install cmake ninja
```

#### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake ninja-build libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype6-dev
```

---

## Step 1: Install vcpkg (Package Manager)

### Windows

```powershell
# Navigate to root of C:\ drive
cd C:\

# Clone vcpkg repository
git clone https://github.com/Microsoft/vcpkg.git

# Bootstrap vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat

# Verify installation
.\vcpkg.exe version
```

### macOS / Linux

```bash
# Clone vcpkg to home directory
cd ~
git clone https://github.com/Microsoft/vcpkg.git

# Bootstrap vcpkg
cd vcpkg
./bootstrap-vcpkg.sh

# Verify installation
./vcpkg version
```

**Expected Output**:
```
vcpkg package management program version 2024.XX.XX
```

---

## Step 2: Install Skia Graphics Library

### Windows

```powershell
cd C:\vcpkg

# Install Skia for x64 Windows
.\vcpkg.exe install skia:x64-windows

# This will take 30-60 minutes (downloads and compiles Skia)
# Expected size: ~2-3 GB
```

### macOS

```bash
cd ~/vcpkg

# Install Skia for macOS
./vcpkg install skia:x64-osx
```

### Linux

```bash
cd ~/vcpkg

# Install Skia for Linux
./vcpkg install skia:x64-linux
```

**Verification**:
```powershell
# Windows
C:\vcpkg\vcpkg.exe list | findstr skia

# macOS/Linux
~/vcpkg/vcpkg list | grep skia
```

**Expected Output**:
```
skia:x64-windows    m116-6ba2c80...    2D Graphics Library
```

---

## Step 3: Clone Zenith DAW Repository

```bash
# Clone the repository
git clone https://github.com/your-org/zenith-daw.git
cd zenith-daw

# Checkout main branch
git checkout master
```

---

## Step 4: Configure CMake

### Windows (Visual Studio 2026 + Ninja)

```powershell
cd C:\zenith\daw

# Create build directory
mkdir build
cd build

# Configure CMake with Ninja generator
cmake .. `
  -G Ninja `
  -DCMAKE_CXX_COMPILER=cl.exe `
  -DCMAKE_C_COMPILER=cl.exe `
  -DZENITH_ENABLE_SKIA=ON `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DCMAKE_BUILD_TYPE=Release
```

### Windows (Visual Studio 2026 IDE)

```powershell
cmake .. `
  -G "Visual Studio 18 2026" `
  -A x64 `
  -DZENITH_ENABLE_SKIA=ON `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### macOS

```bash
cd zenith-daw
mkdir build && cd build

cmake .. \
  -G Ninja \
  -DZENITH_ENABLE_SKIA=ON \
  -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_BUILD_TYPE=Release
```

### Linux

```bash
cd zenith-daw
mkdir build && cd build

cmake .. \
  -G Ninja \
  -DZENITH_ENABLE_SKIA=ON \
  -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_BUILD_TYPE=Release
```

**Expected Output**:
```
-- Fetching JUCE 8.0.9...
-- Finished setting up juceaide
-- JUCE fetched successfully!
-- ============================================
-- Configuring Skia Integration (vcpkg)
-- ============================================
-- Found unofficial-skia
-- Zenith DAW Configuration
-- Version: 0.1.0
-- Build Type: Release
-- JUCE Version: 8.0.9
-- Configuring done (X.Xs)
-- Generating done (X.Xs)
```

---

## Step 5: Build

### Windows (Ninja)

**Option 1: Using Visual Studio Developer Command Prompt**

```powershell
# Open "x64 Native Tools Command Prompt for VS 2026" from Start Menu
cd C:\zenith\daw\build

# Build with Ninja
ninja ZenithDAW

# Or build with CMake (auto-detects Ninja)
cmake --build . --target ZenithDAW --config Release
```

**Option 2: Using Build Script** (Recommended)

```powershell
cd C:\zenith\daw

# Create and run build script
@'
@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cd build
ninja ZenithDAW
echo.
echo Build complete!
if exist zenith-core\ZenithDAW_artefacts\Release\Zenith DAW.exe (
    dir "zenith-core\ZenithDAW_artefacts\Release\Zenith DAW.exe"
)
'@ | Out-File -FilePath build.bat -Encoding ASCII

.\build.bat
```

### macOS / Linux

```bash
cd build

# Build with Ninja
ninja ZenithDAW

# Or with CMake
cmake --build . --target ZenithDAW --config Release
```

**Expected Output**:
```
[1/52] Building CXX object ...
[2/52] Building CXX object ...
...
[52/52] Linking CXX executable ZenithDAW
```

**Build Time**:
- First build: 5-15 minutes
- Incremental builds: 10-60 seconds

---

## Step 6: Locate Executable

### Windows
```
build\zenith-core\ZenithDAW_artefacts\Debug\Zenith DAW.exe   (Debug)
build\zenith-core\ZenithDAW_artefacts\Release\Zenith DAW.exe (Release)
```

### macOS
```
build/zenith-core/ZenithDAW_artefacts/Debug/Zenith DAW.app
build/zenith-core/ZenithDAW_artefacts/Release/Zenith DAW.app
```

### Linux
```
build/zenith-core/ZenithDAW_artefacts/Debug/Zenith DAW
build/zenith-core/ZenithDAW_artefacts/Release/Zenith DAW
```

---

## Step 7: Run

### Windows
```powershell
cd C:\zenith\daw\build\zenith-core\ZenithDAW_artefacts\Release
.\Zenith` DAW.exe
```

### macOS
```bash
open build/zenith-core/ZenithDAW_artefacts/Release/Zenith\ DAW.app
```

### Linux
```bash
./build/zenith-core/ZenithDAW_artefacts/Release/Zenith\ DAW
```

---

## Troubleshooting

### Issue: `Skia package not found via vcpkg!`

**Cause**: CMake can't find Skia in vcpkg installation

**Fix**:
```powershell
# Verify Skia installed
C:\vcpkg\vcpkg.exe list | findstr skia

# If not found, install it
C:\vcpkg\vcpkg.exe install skia:x64-windows

# Reconfigure CMake with correct path
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

---

### Issue: `Could NOT find unofficial-skia (missing: unofficial-skia_DIR)`

**Cause**: Wrong vcpkg triplet (x86 instead of x64)

**Fix**:
```powershell
# Check installed triplets
C:\vcpkg\vcpkg.exe list

# If shows x86-windows, install x64 version
C:\vcpkg\vcpkg.exe install skia:x64-windows

# Reconfigure CMake
cd build
rm CMakeCache.txt
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

---

### Issue: `ninja: no work to do.` but no executable

**Cause**: Build target name mismatch

**Fix**:
```powershell
# List available targets
ninja -t targets

# Build correct target
ninja ZenithDAW
```

---

### Issue: `MSVC compiler not found`

**Cause**: Not running in Visual Studio Developer Command Prompt

**Fix**:

**Option 1**: Open "x64 Native Tools Command Prompt for VS 2026" from Start Menu

**Option 2**: Set up environment in PowerShell:
```powershell
# VS 2026
& "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

# VS 2022
& "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
```

---

### Issue: Build fails with `file not found` errors

**Cause**: CMake lists files that don't exist in repository

**Fix**:
```powershell
# Check which files are missing
# Common culprits:
# - Source/ui/skia/SkiaWaveformRenderer.cpp
# - Source/ui/skia/SkiaClipRenderer.cpp

# Option 1: Create placeholder files
New-Item -Path "zenith-core\Source\ui\skia\SkiaWaveformRenderer.cpp" -ItemType File

# Option 2: Comment out in CMakeLists.txt
# Edit cmake/SkiaManualIntegration.cmake and comment out missing files
```

---

### Issue: OpenGL context fails to initialize

**Cause**: Graphics driver issue or headless environment

**Fix**:
- Update graphics drivers to latest version
- On virtual machines: Enable 3D acceleration
- On Linux: Install mesa OpenGL libraries
  ```bash
  sudo apt install libgl1-mesa-glx libgl1-mesa-dev
  ```

---

## Build Modes

### Debug Build (Default)
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=...
ninja ZenithDAW
```
- **Pros**: Full debug symbols, assertions enabled
- **Cons**: Slower performance (10-50x), larger binary (~100MB)

### Release Build (Recommended for Performance Testing)
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=...
ninja ZenithDAW
```
- **Pros**: Optimized code, fast performance
- **Cons**: No debug symbols, harder to debug crashes

### RelWithDebInfo (Best of Both)
```bash
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_TOOLCHAIN_FILE=...
ninja ZenithDAW
```
- **Pros**: Optimized code + debug symbols
- **Cons**: Larger binary size

---

## Clean Build

```powershell
# Windows
cd C:\zenith\daw
rd /s /q build
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
ninja ZenithDAW
```

```bash
# macOS/Linux
cd zenith-daw
rm -rf build
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake
ninja ZenithDAW
```

---

## Environment Variables

### Optional: Set vcpkg Root Globally

**Windows**:
```powershell
# Add to system environment variables
setx VCPKG_ROOT "C:\vcpkg"

# Then use in CMake:
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
```

**macOS/Linux**:
```bash
# Add to ~/.bashrc or ~/.zshrc
export VCPKG_ROOT=~/vcpkg

# Then use in CMake:
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

---

## Disable Skia (Fallback to JUCE Rendering)

If Skia integration has issues, build without it:

```bash
cmake .. -DZENITH_ENABLE_SKIA=OFF
ninja ZenithDAW
```

**Note**: This uses JUCE's native rendering instead of Skia.

---

## Platform-Specific Notes

### Windows
- **MSVC Compiler**: Requires Visual Studio 2022 or 2026
- **OpenGL**: Included with Windows, no extra setup needed
- **Executable Location**: `build\zenith-core\ZenithDAW_artefacts\Release\Zenith DAW.exe`

### macOS
- **Apple Silicon (M1/M2)**: Use `arm64-osx` triplet: `skia:arm64-osx`
- **Minimum OS**: macOS 10.15 (Catalina)
- **Code Signing**: May need to disable Gatekeeper for unsigned builds
  ```bash
  sudo spctl --master-disable
  ```

### Linux
- **OpenGL**: Install mesa libraries (`libgl1-mesa-dev`)
- **Audio**: ALSA or JACK required
  ```bash
  sudo apt install libasound2-dev libjack-jackd2-dev
  ```
- **X11**: Required for windowing
  ```bash
  sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev
  ```

---

## Quick Reference

| Platform | vcpkg Path | Toolchain File |
|----------|------------|----------------|
| Windows  | `C:\vcpkg` | `C:/vcpkg/scripts/buildsystems/vcpkg.cmake` |
| macOS    | `~/vcpkg`  | `~/vcpkg/scripts/buildsystems/vcpkg.cmake` |
| Linux    | `~/vcpkg`  | `~/vcpkg/scripts/buildsystems/vcpkg.cmake` |

| Build Tool | Command |
|------------|---------|
| Ninja      | `ninja ZenithDAW` |
| Make       | `make ZenithDAW` |
| CMake      | `cmake --build . --target ZenithDAW` |
| Visual Studio | Open `ZenithDAW.sln` and build from IDE |

---

## Next Steps After Build

1. **Run the Application**: See Step 7 above
2. **Test Skia Rendering**: UI should have glassmorphism style with smooth animations
3. **Check Console Output**: Look for "Skia framebuffer rendering initialized successfully!"
4. **Report Issues**: https://github.com/your-org/zenith-daw/issues

---

## Additional Resources

- **JUCE Documentation**: https://juce.com/learn/documentation
- **Skia Documentation**: https://skia.org/docs/
- **vcpkg Documentation**: https://vcpkg.io/en/getting-started.html
- **CMake Documentation**: https://cmake.org/documentation/

---

## Getting Help

- **Build Issues**: Check #build channel in Discord
- **Skia Integration**: See `CRITICAL_FIXES_APPLIED.md`
- **General Support**: Open an issue on GitHub

---

**Happy Building!** 🚀

