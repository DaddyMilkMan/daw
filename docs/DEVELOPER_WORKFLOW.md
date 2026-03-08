# Zenith DAW — Developer Workflow Guide

**Windows-first development workflow for building, debugging, and testing Zenith DAW.**

This guide covers the recommended development setup on Windows using Visual Studio 2022, CMake, and optional alternative IDEs.

---

## Table of Contents

1. [Windows-First Workflow](#windows-first-workflow)
2. [Visual Studio 2022 Workflow](#visual-studio-2022-workflow-recommended)
3. [CMake GUI Workflow](#cmake-gui-workflow)
4. [Alternative IDEs](#alternative-ides-optional)
5. [Testing and Debugging](#testing-and-debugging)
6. [Profiling and Performance](#profiling-and-performance)
7. [Git Workflow](#git-workflow)

---

## Windows-First Workflow

**We DO NOT require WSL or a Linux subsystem to build or run Zenith on Windows.**

Zenith is a native Windows application built with industry-standard tools:
- **Visual Studio 2022** (MSVC v143 compiler)
- **CMake** for build configuration
- **Native Windows APIs** (WASAPI, ASIO, MMCSS)

### Why Native Windows Only?

1. **Performance:** Direct access to Windows audio APIs without translation layers
2. **Audio Driver Compatibility:** Full support for ASIO, WASAPI Exclusive, and MMCSS
3. **Industry Standard:** Professional audio software on Windows uses MSVC
4. **Simpler Builds:** No cross-compilation, no compatibility layers
5. **Better Debugging:** Native debugging with Visual Studio's excellent C++ debugger

### Supported Development Environments

- **Primary:** Visual Studio 2022 (Community, Professional, or Enterprise)
- **Minimal:** Build Tools for Visual Studio 2022 + CMake CLI
- **Optional:** CLion, VS Code (see [Alternative IDEs](#alternative-ides-optional))

---

## Visual Studio 2022 Workflow (Recommended)

### Initial Setup

1. **Install Visual Studio 2022**
   - Download: https://visualstudio.microsoft.com/downloads/
   - Select **"Desktop development with C++"** workload
   - Includes: MSVC compiler, Windows SDK, CMake, Ninja

2. **Clone the Repository**
   ```cmd
   git clone https://github.com/DaddyMilkMan/zenith-core.git
   cd zenith-core
   ```

3. **Open the Project in Visual Studio**

   **Method A: Open CMake Project Directly**
   - Launch Visual Studio 2022
   - **File** → **Open** → **CMake...**
   - Select `CMakeLists.txt` in the `zenith-core/` folder
   - Visual Studio will automatically configure the project

   **Method B: Open Folder**
   - **File** → **Open** → **Folder...**
   - Select the `zenith-core/` folder
   - Visual Studio detects `CMakeLists.txt` automatically

### Building in Visual Studio

1. **Select Configuration**
   - Top toolbar: Choose **Debug** or **Release** from the dropdown

2. **Build the Project**
   - **Build** → **Build All** (or press `Ctrl+Shift+B`)
   - Or right-click the project in Solution Explorer → **Build**

3. **Output Location**
   - Debug: `out/build/x64-Debug/Zenith.exe`
   - Release: `out/build/x64-Release/Zenith.exe`
   - (Path may vary based on VS configuration)

### Running and Debugging

1. **Set Startup Item**
   - Top toolbar: Select **Zenith.exe** from the startup item dropdown

2. **Run Without Debugging**
   - **Debug** → **Start Without Debugging** (or press `Ctrl+F5`)

3. **Run With Debugging**
   - **Debug** → **Start Debugging** (or press `F5`)
   - Set breakpoints by clicking left margin in code editor

### Debugging Tips

- **Breakpoints:** Click left margin or press `F9` on a line
- **Watch Variables:** Hover over variables or add to Watch window
- **Call Stack:** **Debug** → **Windows** → **Call Stack** (`Ctrl+Alt+C`)
- **Autos Window:** **Debug** → **Windows** → **Autos** (shows local variables)
- **Output Window:** **View** → **Output** (`Ctrl+Alt+O`) for build logs

### Visual Studio CMake Settings

To customize CMake options in Visual Studio:

1. **Project** → **CMake Settings for Zenith**
2. Or edit `CMakeSettings.json` directly
3. Common options:
   ```json
   {
     "configurations": [
       {
         "name": "x64-Debug",
         "generator": "Ninja",
         "configurationType": "Debug",
         "cmakeCommandArgs": "-DZENITH_ENABLE_MMCSS=ON"
       }
     ]
   }
   ```

---

## CMake GUI Workflow

For developers who prefer CMake GUI over Visual Studio integration:

### Initial Configuration

1. **Install CMake GUI**
   - Included with Visual Studio 2022, or download from https://cmake.org/download/

2. **Launch CMake GUI**
   - **Where is the source code:** Browse to `zenith-core/`
   - **Where to build the binaries:** Browse to `zenith-core/build/`

3. **Configure**
   - Click **Configure**
   - Select generator: **Visual Studio 17 2022**
   - Click **Finish**
   - CMake will fetch JUCE automatically (first run takes ~2 minutes)

4. **Set Options** (optional)
   - Modify options like `ZENITH_ENABLE_MMCSS` (default: ON)
   - Click **Configure** again to apply changes

5. **Generate**
   - Click **Generate** to create Visual Studio solution files

6. **Open in Visual Studio**
   - Click **Open Project** to launch Visual Studio
   - Or manually open `build/Zenith.sln`

### Building from Command Line

After generating with CMake GUI:

```cmd
cmake --build build --config Debug -j
```

Or use Visual Studio to build the generated solution.

---

## Alternative IDEs (Optional)

### CLion (JetBrains)

**Setup:**
1. **File** → **Open** → Select `zenith-core/` folder
2. CLion detects `CMakeLists.txt` automatically
3. **File** → **Settings** → **Build, Execution, Deployment** → **Toolchains**
   - Ensure **Visual Studio** toolchain is selected
4. **File** → **Settings** → **Build, Execution, Deployment** → **CMake**
   - Add CMake options: `-DZENITH_ENABLE_MMCSS=ON`

**Building:**
- **Build** → **Build Project** (`Ctrl+F9`)
- **Run** → **Run 'Zenith'** (`Shift+F10`)
- **Run** → **Debug 'Zenith'** (`Shift+F9`)

**Notes:**
- CLion uses its own CMake and Ninja (bundled)
- Excellent C++ refactoring tools
- Integrated debugger with visual breakpoints

### Visual Studio Code

**Setup:**
1. Install extensions:
   - **C/C++** (Microsoft)
   - **CMake Tools** (Microsoft)
   - **CMake** (twxs)

2. Open `zenith-core/` folder in VS Code

3. Configure CMake Kit:
   - `Ctrl+Shift+P` → **CMake: Select a Kit**
   - Choose **Visual Studio Community 2022 Release - amd64**

4. Configure CMake:
   - `Ctrl+Shift+P` → **CMake: Configure**
   - CMake Tools will fetch JUCE automatically

**Building:**
- `Ctrl+Shift+P` → **CMake: Build** (or press `F7`)
- Or click **Build** in the bottom status bar

**Debugging:**
- Set breakpoints (click left margin)
- `F5` to start debugging
- Or `Ctrl+Shift+P` → **CMake: Debug**

**Notes:**
- Lightweight alternative to Visual Studio
- Requires manual setup compared to VS 2022
- Good for quick edits and builds

---

## Testing and Debugging

### Audio Thread Debugging

**IMPORTANT:** The audio callback runs in a real-time thread with hard timing constraints.

**Best Practices:**
1. **DO NOT** set breakpoints in audio callback functions
   - Breaks real-time guarantees
   - Causes audio glitches and dropouts
2. **Use logging** for audio thread debugging:
   - Write to lock-free FIFO
   - Process logs on message thread
3. **Debug audio logic offline** with unit tests

### Visual Studio Debugger Features

**Conditional Breakpoints:**
1. Set breakpoint (`F9`)
2. Right-click breakpoint → **Conditions...**
3. Add condition (e.g., `frameCounter > 1000`)

**Data Breakpoints:**
- Break when a variable changes value
- Right-click variable → **Break When Value Changes**

**Exception Settings:**
- **Debug** → **Windows** → **Exception Settings** (`Ctrl+Alt+E`)
- Enable "Break When Thrown" for C++ exceptions

### Performance Debugging

**Visual Studio Profiler:**
1. **Debug** → **Performance Profiler** (`Alt+F2`)
2. Select tools:
   - **CPU Usage** (function-level profiling)
   - **Memory Usage** (heap allocations)
3. Click **Start**
4. Interact with Zenith
5. Click **Stop Collection**
6. Analyze hot paths and allocations

**ETW (Event Tracing for Windows):**
- Use **Windows Performance Analyzer (WPA)** for deep profiling
- See [Profiling and Performance](#profiling-and-performance)

---

## Profiling and Performance

### CPU Profiling

**Visual Studio CPU Profiler (Quick):**
- **Debug** → **Performance Profiler** → **CPU Usage**
- Shows per-function CPU time
- Good for finding hot paths

**ETW + WPA (Advanced):**
1. Install **Windows Performance Toolkit** (included with Windows SDK)
2. Record trace:
   ```cmd
   wpr -start CPU
   # Run Zenith and perform operations
   wpr -stop trace.etl
   ```
3. Open `trace.etl` in **Windows Performance Analyzer**
4. Analyze CPU usage, context switches, scheduler events

### Memory Profiling

**Visual Studio Memory Profiler:**
- **Debug** → **Performance Profiler** → **Memory Usage**
- Take snapshots before/after operations
- Compare snapshots to find leaks

**Heap Profiling:**
- Enable **Allocation Profiling** in debug builds
- Check for allocations on audio thread (NEVER allowed!)

### Audio Performance Metrics

**Track in Zenith Debug HUD (planned):**
- Frame time (16.67ms target for 60fps)
- Paint calls per second
- Audio buffer underruns
- CPU usage %

**PresentMon (Frame Rate Analysis):**
- Download: https://github.com/GameTechDev/PresentMon
- Measure frame pacing and latency
- Useful for UI performance tuning

---

## Git Workflow

### Branching

**Main Branches:**
- `main` — Stable releases
- `develop` — Integration branch for features

**Feature Branches:**
- Create from `develop`:
  ```cmd
  git checkout develop
  git pull origin develop
  git checkout -b feature/my-new-feature
  ```

**Branch Naming:**
- `feature/feature-name` — New features
- `fix/bug-description` — Bug fixes
- `docs/topic` — Documentation updates
- `refactor/component-name` — Code refactoring

### Committing Changes

**Commit Messages:**
- Use present tense: "Add feature" not "Added feature"
- Be specific: "Fix transport play button state sync" not "Fix bug"
- Reference issues: "Fix #123: Audio dropout on buffer resize"

**Example:**
```cmd
git add Source/ui/TransportBar.cpp
git commit -m "Fix transport play button state sync

- Update playButton toggle state on engine callback
- Add test for play/pause state consistency
- Resolves #123"
```

### Pulling Latest Changes

```cmd
git checkout develop
git pull origin develop
git checkout feature/my-feature
git merge develop
```

Or use rebase for cleaner history:
```cmd
git checkout feature/my-feature
git rebase develop
```

### Pushing and Pull Requests

```cmd
git push origin feature/my-feature
```

Then create a Pull Request on GitHub:
- Base branch: `develop`
- Compare branch: `feature/my-feature`
- Fill in PR template with summary and test plan

---

## Code Style and Best Practices

### C++ Style

- **Follow JUCE conventions:**
  - CamelCase for classes: `TrackView`, `TransportBar`
  - camelCase for functions: `playButtonClicked()`, `updateMeters()`
  - Use `jassert()` for debug assertions

- **Real-time safety:**
  - **NEVER** allocate/deallocate on audio thread
  - **NEVER** lock mutexes on audio thread
  - **NEVER** make system calls on audio thread
  - See [docs/tech-briefs/06-audio-thread-safety-policy.md](tech-briefs/06-audio-thread-safety-policy.md)

### Pre-commit Checks

Before committing:
1. **Build succeeds** (Debug and Release)
2. **No compiler warnings** (warnings are errors in CI)
3. **Code formatted** (clang-format, if configured)
4. **Changes tested** manually or with unit tests

---

## Quality Standards

### The "No Stubs" Policy
Zenith DAW adheres to a strict "No Stubs" policy for all committed code.
- **UI Components:** Must render correctly and respond to interactions. Do not commit blank panels or non-functional buttons.
- **Audio Classes:** Must implement actual processing logic. Empty `processBlock` methods are not allowed for active features.
- **Backend Logic:** Must be fully wired. If you add a "Scan Plugins" button, it must actually scan plugins, not just print a log message.

### "No Shortcuts" Engineering
- **Thread Safety:** Never compromise on thread safety for speed of implementation. Use lock-free structures (FIFOs, atomics) for audio threads.
- **Error Handling:** Handle edge cases (file not found, device disconnected) gracefully. Do not assume "happy path".
- **Architecture:** Respect the `ProjectState` (Data) vs `Engine` (Audio) vs `Component` (UI) separation. Do not bypass architectural layers for convenience.

---

## Troubleshooting Development Issues

### "CMake Error: Could not find JUCE"

**Fix:** Delete `build/` and reconfigure:
```cmd
rmdir /s /q build
cmake -B build -G "Visual Studio 17 2022"
```

### "LNK1104: cannot open file 'juce_*.lib'"

**Fix:** Clean rebuild:
```cmd
cmake --build build --config Debug --clean-first
```

### Visual Studio Intellisense Errors (Red Squiggles)

**Fix:**
1. **Project** → **Rescan Solution**
2. Or delete `.vs/` folder and reopen project
3. Ensure CMake configuration succeeded (check Output window)

### "Audio device initialization failed"

**Fix:**
1. Close other audio applications (DAWs, browsers with media)
2. Check Windows audio settings (ensure device is enabled)
3. Try different buffer size in Zenith audio settings
4. Check MMCSS is enabled: `cmake -B build -DZENITH_ENABLE_MMCSS=ON`

---

## Additional Resources

### Official Documentation
- **JUCE Tutorials:** https://docs.juce.com/master/tutorial_manage_plugins.html
- **JUCE Forum:** https://forum.juce.com/
- **Windows Audio APIs:** [docs/WINDOWS_AUDIO_APIS_GUIDE.md](WINDOWS_AUDIO_APIS_GUIDE.md)
- **MMCSS Audio Priority:** [docs/windows/mmcss_audio_priority.md](windows/mmcss_audio_priority.md)

### Community
- **Audio Developer Conference:** https://audio.dev/
- **KVR Audio DSP Forum:** https://www.kvraudio.com/forum/viewforum.php?f=33

### Books
- *Designing Audio Effect Plugins in C++* by Will Pirkle
- *The Audio Programming Book* by Richard Boulanger

---

## Quick Reference

### Common Commands

```cmd
# Clone repository
git clone https://github.com/DaddyMilkMan/zenith-core.git

# Configure CMake
cmake -B build -G "Visual Studio 17 2022"

# Build Debug
cmake --build build --config Debug -j

# Build Release
cmake --build build --config Release -j

# Clean rebuild
rmdir /s /q build
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug -j

# Run Debug build
build\Debug\Zenith.exe

# Run Release build
build\Release\Zenith.exe
```

### CMake Options

```cmd
# Enable MMCSS audio priority (default: ON)
-DZENITH_ENABLE_MMCSS=ON

# Enable Link-Time Code Generation (default: OFF)
-DZENITH_LTCG=ON

# Treat warnings as errors (CI only, default: OFF)
-DZENITH_WERROR_CI=ON
```

### Visual Studio Shortcuts

| **Action** | **Shortcut** |
|------------|--------------|
| Build | `Ctrl+Shift+B` |
| Start Debugging | `F5` |
| Start Without Debugging | `Ctrl+F5` |
| Toggle Breakpoint | `F9` |
| Step Over | `F10` |
| Step Into | `F11` |
| Step Out | `Shift+F11` |
| Go to Definition | `F12` |
| Find in Files | `Ctrl+Shift+F` |
| Solution Explorer | `Ctrl+Alt+L` |
| Output Window | `Ctrl+Alt+O` |

---

**Questions or issues?** Open a GitHub issue or check the [main README](../README.md).

**Happy coding!** 🎵
