# Zenith DAW - macOS Installation Guide

**Last Updated:** December 28, 2025

## Prerequisites

### System Requirements
- **OS:** macOS 10.15+ (Catalina and newer)
- **Architecture:** Intel x64 or Apple Silicon (M1/M2/M3)
- **RAM:** 8GB+ minimum, 16GB+ recommended  
- **Storage:** 4GB+ free space
- **Xcode:** 14+ (for command line tools)

### Required Tools
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew (if not installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake git ninja vcpkg
```

## Installation Steps

### 1. Install vcpkg
```bash
# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg
cd ~/vcpkg

# Bootstrap
./bootstrap-vcpkg.sh

# Add to PATH (add to ~/.zshrc or ~/.bash_profile)
echo 'export PATH="$HOME/vcpkg:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

### 2. Install Skia Dependencies
```bash
# Install Skia (this may take 30+ minutes)
vcpkg install skia:x64-osx

# Additional system dependencies
vcpkg install \
    jack \
    libuuid \
    openssl
```

### 3. Clone Zenith DAW
```bash
# Clone repository
git clone [repository-url] ~/zenith
cd ~/zenith/daw
```

### 4. Build
```bash
# Create build directory
mkdir build && cd build

# Configure CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DZENITH_USE_SKIA=ON \
    -DENABLE_ONNX=ON \
    -G Ninja

# Build (use all CPU cores)
ninja

# Alternative: Use make
# cmake .. -G "Unix Makefiles"
# make -j$(sysctl -n hw.ncpu)
```

### 5. Run
```bash
# From build directory
./ZenithDAW

# Or create application bundle (optional)
ninja install
# Then run from open ~/zenith/daw/build/ZenithDAW.app
```

## Troubleshooting

### Build Issues

**"Xcode tools not found"**
```bash
# Install command line tools
xcode-select --install
# Accept license and wait for installation
```

**"CMAKE_OSX_DEPLOYMENT_TARGET not supported"**
```bash
# Update CMake or use minimum version
brew upgrade cmake

# Set deployment target manually
cmake .. -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
```

**"Skia build fails"**
```bash
# Install Xcode for full toolchain
# Or install specific SDKs
sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer

# Clean vcpkg and rebuild
cd ~/vcpkg
./vcpkg remove skia:x64-osx
./vcpkg install skia:x64-osx
```

### Runtime Issues

**"No audio devices found"**
```bash
# Grant microphone permission
# System Preferences → Security & Privacy → Microphone → Add ZenithDAW

# Check audio devices
afinfo -a
# Or with Core Audio
kextstat | grep apple
```

**"Cannot start application"**
```bash
# Gatekeeper bypass (first time only)
sudo xattr -rd com.apple.quarantine ~/zenith/daw/build/ZenithDAW

# Or allow in System Preferences
# System Preferences → Security & Privacy → General → Allow Anyway
```

**"Performance issues"**
```bash
# Set high performance mode
sudo pmset -c high

# Disable Nap (optional)
sudo pmset -c high -a disablesleep 1
```

### Architecture Specific

**Apple Silicon (M1/M2/M3)**
```bash
# Install Rosetta 2 (for Intel plugins)
softwareupdate --install-rosetta --agree-to-license

# Build natively for ARM
vcpkg install skia:arm64-osx

# Cross-compilation not needed for native builds
```

**Intel Macs**
```bash
# Standard x64 build
vcpkg install skia:x64-osx

# Enable AVX2 for better performance
cmake .. -DCMAKE_CXX_FLAGS="-mavx2"
```

## Development Setup

### IDE Configuration
**Xcode:**
```bash
# Generate Xcode project
cmake .. -G Xcode

# Open project
open ZenithDAW.xcodeproj
```

**VS Code:**
```bash
# Install extensions
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.cmake-tools

# Configure workspace
cd ~/zenith/daw
code .
```

**CLion:**
- Open CMakeLists.txt as project
- Set build directory to `build`
- Configure toolchain to vcpkg

## Code Signing

### Development Builds
```bash
# Disable code signing for development
export CODE_SIGNING_ALLOWED=NO
cmake .. -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
```

### Distribution Builds
```bash
# Sign with developer ID
codesign --force --verify --verbose --sign "Developer ID Application: Your Name" \
    ~/zenith/daw/build/ZenithDAW.app

# Notarize for distribution
xcrun altool --notarize-app --primary-bundle-id "com.zenithaudio.daw" \
    --username "your@email.com" --password "@keychain:AC_PASSWORD" \
    --file ZenithDAW.pkg
```

## Testing

### Run Unit Tests
```bash
cd ~/zenith/daw/build
ninja ZenithDAWTests
./ZenithDAWTests
```

### Audio Backend Test
```bash
# Test Core Audio
afinfo -a
# Play test sound
afplay ~/zenith/daw/Content/Examples/audio/test.wav
```

## Performance Optimization

### Faster Builds
```bash
# Use ccache
brew install ccache
export CC="ccache clang"
export CXX="ccache clang++"

# Use Ninja instead of make
cmake .. -G Ninja
```

### Optimize Binary Size
```bash
# Build with size optimization
cmake .. -DCMAKE_BUILD_TYPE=MinSizeRel

# Enable LTO (Link Time Optimization)
cmake .. -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

## Package Creation

### DMG Package
```bash
# Install create-dmg
brew install create-dmg

# Create distributable package
create-dmg \
    --volname "Zenith DAW" \
    --window-pos 200 120 \
    --window-size 600 300 \
    --icon-size 100 \
    --icon ~/zenith/daw/apps/desktop/Resources/icons/zenith.icns \
    --hide-extension "ZenithDAW" \
    --app-drop-link 450 10 \
    ~/zenith/daw/build/ZenithDAW.app \
    ZenithDAW.dmg
```

## Next Steps

After successful installation:
1. **Configure Audio:** Open Zenith DAW → Settings → Audio Device  
2. **Grant Permissions:** System Preferences → Security & Privacy → Microphone/Folders
3. **Scan Plugins:** Tools → Plugin Manager → Scan for VST3/AU plugins
4. **Load Demo Project:** File → Open → `Content/Examples/`

---
**Need help?** Check the main README.md or open an issue on GitHub.