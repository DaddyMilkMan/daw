# Zenith DAW

A digital audio workstation built with C++20 and JUCE. It's fast, straightforward, and designed for musicians who want tools that stay out of their way.

**Status:** Early alpha development - Linux first, cross platform goal

## What's This About?

Zenith DAW is a digital audio workstation for musicians, producers, and audio engineers. I'm building it from scratch with modern C++ to be performant and reliable. The goal is to create something that works well without getting in your creative process.

### Why I Started This

I kept running into DAWs that felt bloated, locked me in, or just didn't work the way I wanted. So I decided to build my own. Something that respects your time, doesn't force you into a specific workflow, and actually performs well on modern hardware.

## Getting Started

### First Time Building

1. **Clone the repo**
   ```bash
   git clone https://github.com/micahcooley/daw.git
   cd daw
   ```

2. **Install dependencies**
   * **Linux/Ubuntu:**
     ```bash
     sudo apt install build-essential cmake ninja-build libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev libfreetype6-dev libx11-dev libxinerama-dev libxext-dev libxrandr-dev libxcursor-dev libwebkit2gtk-4.0-dev libglu1-mesa-dev mesa-common-dev
     ```
   * **macOS:**
     ```bash
     xcode-select --install
     brew install cmake ninja
     ```
   * **Windows:** Install Visual Studio 2022 with C++ workload

3. **Build it**
   ```bash
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   cmake --build build -j$(nproc)
   ```

4. **Run it (Linux currently)**
   ```bash
   ./build/Zenith\ DAW
   ```

Note: Currently Linux-focused but designed for cross platform use. Windows and macOS support is in progress.

## What Actually Works

What I have working so far:

* Basic audio I/O on Linux
* Some MIDI functionality
* Built in synthesizer components (structure exists)
* Project file save/load (basic functionality)
* Plugin hosting infrastructure (VST3 support is built into JUCE, I'm building the host code)

Note: VST3 plugin hosting works through JUCE's built in support, but I'm still building the complete plugin management system.

What's mostly structure right now:

* Audio engine architecture
* Cross platform UI framework
* Advanced features like stem separation and AI

## Current Focus Areas

* Getting VST3 plugin hosting working reliably
* Fixing thread safety in audio engine
* Implementing actual macOS/Windows support
* Basic AI integration (currently just API calls)
* Stem separation (needs ONNX runtime)

## How to Help

I'd appreciate help from all kinds of people:

### If You Code

* Bug fixes in existing components
* Adding VST3 plugin support
* Optimizing audio processing routines
* Improving the UI/UX design
* Writing tests

### If You Make Music

* Test builds and report issues
* Suggest features that would help your workflow
* Create presets and share them
* Help make the software more intuitive

### If You're Just Interested

* Improve documentation
* Translate the interface
* Share the project with others
* Report bugs and suggest improvements

## Project Layout

```
├── apps/desktop/        # Desktop app
│   ├── ai_client/       # AI integration
│   ├── browser/         # Built in browser
│   ├── platform/        # OS specific code
│   └── tests/           # App tests
├── modules/             # Core C++ modules
│   ├── zenith_core/     # Audio engine and DSP
│   ├── zenith_ui/       # UI components
│   ├── zenith_network/  # Collaboration features
│   └── zenith_commands/ # Command system
├── docs/                # Documentation
└── Content/            # Presets and samples
```

## Contact

This is a very early project. Contact me on GitHub:

* **GitHub Issues:** Report bugs or ask questions

## License

This is licensed under AGPL v3. In simple terms:

* You can use it commercially
* You can modify and share the source code
* Your changes must also be open source
* Network accessed versions need to provide source code

I also offer commercial licenses if you need something proprietary.

## Why AGPL v3?

I wanted to make sure Zenith stays open and free forever. AGPL v3 means improvements and innovations get shared back with everyone, while still allowing commercial use. It's about keeping the project open for future musicians.

## About This Project

I develop on Linux since that's my main environment, which is why most of the current work is Linux-focused. The goal is cross platform compatibility though; this is being built with multi platform support in mind from the start.