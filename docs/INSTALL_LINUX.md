# Zenith DAW - Linux Installation Guide

**Last Updated:** December 28, 2025

## Prerequisites

### System Requirements
- **OS:** Ubuntu 20.04+ / Fedora 35+ / Arch Linux
- **Architecture:** x64 (ARM64 experimental)
- **RAM:** 8GB+ minimum, 16GB+ recommended
- **Storage:** 2GB free space

### Required Packages
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    libasound2-dev \
    libjack-jackd2-dev \
    ladspa-sdk \
    lv2-dev \
    pkg-config

# Fedora
sudo dnf install -y \
    gcc-c++ \
    cmake \
    git \
    alsa-lib-devel \
    jack-audio-connection-kit-devel \
    ladspa-devel \
    lv2-devel \
    pkgconfig

# Arch Linux
sudo pacman -S --needed \
    base-devel \
    cmake \
    git \
    alsa-lib \
    jack \
    ladspa \
    lv2 \
    pkgconf
```

## Installation Steps

### 1. Install vcpkg
```bash
# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg
cd ~/vcpkg

# Bootstrap
./bootstrap-vcpkg.sh

# Add to PATH (add to ~/.bashrc or ~/.zshrc)
echo 'export PATH="$HOME/vcpkg:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

### 2. Install Skia Dependencies
```bash
# Install Skia (this may take 30+ minutes)
vcpkg install skia:x64-linux

# Additional audio dependencies
vcpkg install \
    alsa \
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
    -DENABLE_ONNX=ON

# Build (use all CPU cores)
make -j$(nproc)

# Alternative: Use Ninja for faster builds
# Install ninja: sudo apt install ninja-build
# Then: cmake .. -G Ninja
# Build: ninja
```

### 5. Run
```bash
# From build directory
./ZenithDAW

# Or install system-wide (optional)
sudo make install
```

## Troubleshooting

### Build Issues

**"CMAKE_CXX_COMPILER not set"**
```bash
# Install GCC explicitly
sudo apt install gcc g++ cmake
export CC=gcc
export CXX=g++
cmake ..
```

**"Skia not found"**
```bash
# Check vcpkg installation
vcpkg list | grep skia

# Reinstall if needed
vcpkg remove skia:x64-linux
vcpkg install skia:x64-linux
```

**"OpenGL errors"**
```bash
# Install graphics drivers
sudo ubuntu-drivers autoinstall
# Or specific:
sudo apt install nvidia-driver-470  # NVIDIA
sudo apt install mesa-vulkan-drivers  # AMD/Intel
```

### Runtime Issues

**"No audio devices found"**
```bash
# Add user to audio group
sudo usermod -a -G audio $USER
# Logout and login again

# Check ALSA devices
aplay -l
# Check JACK
jackd -d
```

**"Permission denied"**
```bash
# Fix permissions
chmod +x ~/zenith/daw/build/ZenithDAW
sudo chown $USER:$USER ~/zenith/daw/build -R
```

### Performance Optimization

**Reduce RAM usage:**
```bash
# Build with -DCMAKE_BUILD_TYPE=RelWithDebInfo for smaller binaries
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

**Faster builds:**
```bash
# Use ccache
sudo apt install ccache
export CC="ccache gcc"
export CXX="ccache g++"
```

## Development Setup

### IDE Configuration
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

## Testing

### Run Unit Tests
```bash
cd ~/zenith/daw/build
make ZenithDAWTests
./ZenithDAWTests
```

### Audio Backend Test
```bash
# Test ALSA
aplay ~/zenith/daw/Content/Examples/audio/test.wav

# Test JACK
jackd -d && ~/zenith/daw/build/ZenithDAW
```

## Next Steps

After successful installation:
1. **Configure Audio:** Open Settings → Audio Device
2. **Scan Plugins:** Tools → Plugin Manager → Scan
3. **Load Demo Project:** File → Open → `Content/Examples/`

---
**Need help?** Check the main README.md or open an issue on GitHub.