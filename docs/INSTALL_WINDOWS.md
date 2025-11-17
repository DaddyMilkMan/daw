# Windows Installation Guide — Zenith DAW

**Native Windows Development • No WSL Required • No MSYS2 Required • No vcpkg Required**

Zenith DAW is built for native Windows development using Visual Studio 2022 and CMake. JUCE is automatically fetched during the CMake configure step—no manual dependency management needed.

---

## Requirements

### Required Software

| Tool | Version | Purpose |
|------|---------|---------|
| **Windows** | 10 or 11 | Operating system |
| **Visual Studio 2022** | Community/Pro/Enterprise | C++ compiler (MSVC v143) |
| **CMake** | ≥ 3.22 | Build system generator |
| **Git** | Latest | Version control |

### Visual Studio 2022 Setup

When installing Visual Studio 2022, ensure you select:

- **Workload**: Desktop development with C++
- **Individual Components** (automatically included):
  - MSVC v143 (or latest)
  - Windows 10/11 SDK
  - C++ CMake tools for Windows

> **No additional tools required**: WSL, MSYS2, vcpkg, and Cygwin are NOT needed.

---

## Installation Steps

### 1. Install Prerequisites

#### Install Visual Studio 2022

1. Download from: https://visualstudio.microsoft.com/downloads/
2. Run the installer
3. Select **Desktop development with C++** workload
4. Click **Install**

#### Install CMake

**Option A: Via Visual Studio Installer** (Recommended)
- CMake is included with the "C++ CMake tools for Windows" component

**Option B: Standalone Installer**
1. Download from: https://cmake.org/download/
2. Run installer and select "Add CMake to system PATH"

**Option C: Via Chocolatey**
```powershell
choco install cmake
```

#### Install Git

1. Download from: https://git-scm.com/download/win
2. Run installer with default settings

Or via Chocolatey:
```powershell
choco install git
```

---

### 2. Clone the Repository

Open PowerShell or Command Prompt:

```powershell
# Clone the repository
git clone https://github.com/DaddyMilkMan/daw.git
cd daw
```

---

### 3. Configure and Build

Zenith DAW's native JUCE UI is located in the `zenith-core/` directory.

#### Configure the Build

```powershell
# Navigate to zenith-core
cd zenith-core

# Configure Debug build
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug

# Configure Release build
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release
```

**What happens during configure:**
- CMake automatically fetches JUCE 8.0.9 from GitHub using `FetchContent`
- No manual JUCE installation or submodule setup required
- This may take a few minutes on first run (JUCE is ~150MB)

#### Build the Application

```powershell
# Build Debug
cmake --build build/Debug -j

# Build Release
cmake --build build/Release -j
```

The `-j` flag enables parallel compilation for faster builds.

---

### 4. Run Zenith DAW

After building, the executable will be located in:

```
zenith-core/build/Debug/ZenithDAW_artefacts/Debug/Zenith DAW.exe
zenith-core/build/Release/ZenithDAW_artefacts/Release/Zenith DAW.exe
```

Run it directly:

```powershell
# Debug build
.\build\Debug\ZenithDAW_artefacts\Debug\"Zenith DAW.exe"

# Release build
.\build\Release\ZenithDAW_artefacts\Release\"Zenith DAW.exe"
```

Or double-click the `.exe` in Windows Explorer.

---

## Build Options

Zenith uses several CMake options for customization:

```powershell
# Enable MMCSS audio thread priority (Windows-specific, enabled by default)
cmake -S . -B build -DZENITH_ENABLE_MMCSS=ON

# Enable debug track seeding (creates 8 demo tracks at startup in Debug)
cmake -S . -B build -DZENITH_ENGINE_SEED_DEBUG_TRACKS=ON

# Enable experimental audio skeleton (Phase 1 - not ready)
cmake -S . -B build -DZENITH_ENABLE_PHASE1_AUDIO=OFF
```

---

## Troubleshooting

### "CMake not found"

**Solution:**
- Ensure CMake is installed via Visual Studio Installer or standalone
- Verify it's in your PATH: `cmake --version`
- Restart your terminal after installation

### "MSVC compiler not found"

**Solution:**
- Open **Visual Studio Installer**
- Modify your VS 2022 installation
- Ensure **Desktop development with C++** is checked
- Verify MSVC v143 toolset is installed

### "Cannot open include file 'JuceHeader.h'"

**Solution:**
- JUCE may not have been fetched correctly
- Delete the build directory: `rmdir /s build`
- Re-run CMake configure step
- Check your internet connection (JUCE is fetched from GitHub)

### "Git fetch failed for JUCE"

**Solution:**
- Check your internet connection
- Verify Git is installed: `git --version`
- Try fetching JUCE manually:
  ```powershell
  git clone --depth 1 --branch 8.0.9 https://github.com/juce-framework/JUCE.git _deps/juce
  ```

### Build is slow

**Solution:**
- Use parallel builds: `cmake --build build -j`
- Close other applications
- Use Release build for production (Debug includes symbols)
- Consider upgrading to an SSD if using HDD

---

## Opening in Visual Studio

You can open the project directly in Visual Studio 2022:

**Method 1: Open Folder**
1. File → Open → Folder
2. Select the `zenith-core/` directory
3. Visual Studio will detect `CMakeLists.txt` automatically
4. Use the CMake targets dropdown to select Debug/Release

**Method 2: Generate Solution**
```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
```
Then open `build/ZenithDAW.sln`

---

## Next Steps

- **Audio Configuration**: See [WINDOWS_AUDIO_APIS_GUIDE.md](WINDOWS_AUDIO_APIS_GUIDE.md) for ASIO/WASAPI setup
- **Development Workflow**: See [DEVELOPER_WORKFLOW.md](DEVELOPER_WORKFLOW.md) for coding guidelines and best practices
- **Architecture**: See the main [README.md](../README.md) for project structure and roadmap

---

## Quick Reference

```powershell
# Full build from scratch
cd zenith-core
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release -j
.\build\Release\ZenithDAW_artefacts\Release\"Zenith DAW.exe"
```

**Build time (first run):** ~5-10 minutes (includes JUCE fetch + compile)
**Build time (incremental):** ~10-30 seconds (depending on changes)

---

**Last Updated:** 2025-11-17
**Tested With:** Windows 11, Visual Studio 2022 17.8+, CMake 3.27
