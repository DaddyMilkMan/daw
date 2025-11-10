# Zenith Core - JUCE Audio Engine

**Phase 0: Foundation**

This is the core audio engine for Zenith DAW, built with JUCE 8.0.9 and C++20.

## Features (Phase 0)

✅ **Implemented:**
- JUCE 8.0.9 audio engine
- Audio device management (automatic device selection)
- Real-time audio processing with thread safety
- Transport controls (play/stop)
- CPU usage monitoring
- ValueTree state management
- Undo/redo system
- Project save/load (XML format)
- Test tone generator (440 Hz sine wave)

🚧 **Coming in Phase 1:**
- Multi-track recording
- VST3 plugin hosting
- MIDI input/output
- Timeline view
- Audio waveform rendering

## Build Requirements

### All Platforms
- CMake 3.22 or higher
- C++20 compatible compiler
- Internet connection (first build only - to fetch JUCE)

### macOS
- Xcode 13+ (for C++20 support)
- macOS 10.13+ deployment target

### Windows
- Visual Studio 2019+ or MinGW with C++20
- Windows 10+

### Linux
- GCC 10+ or Clang 13+
- ALSA development libraries: `sudo apt install libasound2-dev`
- FreeType: `sudo apt install libfreetype6-dev`
- X11: `sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev`
- WebKit (optional): `sudo apt install libwebkit2gtk-4.0-dev`

## Building

### Quick Start

```bash
# Navigate to zenith-core directory
cd zenith-core

# Create build directory
mkdir build
cd build

# Configure with CMake (fetches JUCE automatically)
cmake ..

# Build
cmake --build . --config Release

# Run
./ZenithDAW  # Linux/macOS
# or
Release/ZenithDAW.exe  # Windows
```

### Build Options

```bash
# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug

# Release build with optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# macOS: Build universal binary (Apple Silicon + Intel)
cmake .. -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build . --config Release

# Specify compiler
cmake .. -DCMAKE_CXX_COMPILER=clang++
cmake --build .

# Verbose build output
cmake --build . --verbose
```

### IDE Support

#### Visual Studio Code
```bash
# Install CMake Tools extension
# Open zenith-core folder
# CMake will auto-configure
# Press F7 to build
```

#### Xcode
```bash
cmake .. -G Xcode
open ZenithDAW.xcodeproj
```

#### Visual Studio
```bash
cmake .. -G "Visual Studio 17 2022"
start ZenithDAW.sln
```

## Project Structure

```
zenith-core/
├── CMakeLists.txt         # Build configuration
├── README.md              # This file
│
├── include/               # Header files
│   ├── Engine.h          # Core audio engine
│   ├── MainWindow.h      # Main UI window
│   └── ProjectState.h    # ValueTree state management
│
├── src/                   # Source files
│   ├── Main.cpp          # Application entry point
│   ├── MainWindow.cpp    # Main UI implementation
│   ├── Engine.cpp        # Audio engine implementation
│   └── ProjectState.cpp  # State management implementation
│
├── resources/             # Resources (icons, fonts)
├── modules/               # Custom JUCE modules (Phase 1+)
└── tests/                 # Unit tests (Phase 1+)
```

## Usage

### First Run

When you run Zenith DAW for the first time:

1. **Audio Device Selection**
   - The engine automatically selects your system's default audio device
   - Check the bottom status bar to see the current device and settings

2. **Test the Audio Engine**
   - Click the **Play** button
   - You should hear a 440 Hz test tone (A4 note) at -12 dB
   - Click **Stop** to silence

3. **Monitor CPU Usage**
   - Top-right corner shows CPU usage percentage
   - Should be < 5% for test tone generation

### Keyboard Shortcuts

- **Space**: Play/Stop (coming in Phase 1)
- **Cmd/Ctrl + N**: New Project (coming in Phase 1)
- **Cmd/Ctrl + O**: Open Project (coming in Phase 1)
- **Cmd/Ctrl + S**: Save Project (coming in Phase 1)

## Thread Safety

The audio engine follows strict real-time safety rules:

### Audio Thread (Real-Time)
✅ **Allowed:**
- Process audio samples
- Read `std::atomic` values
- Use pre-allocated buffers
- Simple math operations

❌ **NEVER:**
- Allocate memory (`new`, `malloc`, `std::vector::push_back`)
- Lock mutexes (`std::mutex`, `std::lock_guard`)
- System calls (file I/O, logging, network)
- Call UI methods

### Message Thread
✅ **Allowed:**
- All of the above
- UI updates
- File I/O
- Network operations
- Memory allocation

## Architecture

### Engine.cpp
- Manages audio device I/O
- Runs real-time audio callback
- Implements transport controls
- Monitors CPU usage

### ProjectState.cpp
- Uses JUCE ValueTree for state management
- Automatic undo/redo support
- Serialization to XML
- Thread-safe with proper listeners

### MainWindow.cpp
- Main UI layout
- Transport controls
- Status monitoring
- Menu bar (Phase 1)

## Development

### Adding New Features

1. **Read the planning docs** in `../planning/`
2. **Check the decision matrix** for priorities
3. **Follow thread safety rules** (see docs/tech-briefs/06-audio-thread-safety-policy.md)
4. **Use ValueTree** for all state changes (enables undo/redo)
5. **Write tests** (Phase 1+)

### Debugging

```bash
# Enable debug logging
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Run with verbose output
./ZenithDAW --verbose  # Not implemented yet

# Use debugger
lldb ./ZenithDAW  # macOS/Linux
# or
gdb ./ZenithDAW  # Linux
```

### Common Issues

**Issue**: CMake can't find JUCE
**Solution**: Ensure you have internet connection. CMake will download JUCE automatically using FetchContent.

**Issue**: No audio output
**Solution**: Check audio device settings. On Linux, ensure ALSA is installed and your user is in the `audio` group.

**Issue**: Compile errors about C++20
**Solution**: Update your compiler. GCC 10+, Clang 13+, or MSVC 2019+ required.

**Issue**: High CPU usage
**Solution**: Check your audio buffer size. Increase it in your system settings (512 or 1024 samples recommended).

## Testing

```bash
# Phase 0: Manual testing
# 1. Run the application
# 2. Click Play - should hear 440 Hz tone
# 3. CPU usage should be < 5%
# 4. Click Stop - should be silent

# Phase 1+: Unit tests
cmake --build . --target tests
./tests/ZenithTests
```

## Contributing

See the main project README and planning documents:
- `../planning/README.md` - Planning overview
- `../planning/roadmaps/MASTER_IMPLEMENTATION_ROADMAP.md` - Implementation timeline
- `../docs/tech-briefs/` - Technical guides

## License

TBD (likely GPL v3 or commercial license)

## Resources

- [JUCE Documentation](https://docs.juce.com)
- [JUCE Forum](https://forum.juce.com)
- [CMake Documentation](https://cmake.org/documentation/)

---

**Zenith DAW** - The Perfect DAW with AI Integration
**Phase 0 Progress**: 40% Complete
**Version**: 0.1.0
