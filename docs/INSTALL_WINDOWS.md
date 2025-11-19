# Zenith DAW — Windows Installation Guide

**Fast, native Windows setup. No WSL, no MSYS2, no package managers.**

This guide covers installing and building Zenith DAW on Windows 10/11 using Visual Studio 2022 and CMake.

---

## Prerequisites

### 1. Operating System
- **Windows 10** (64-bit) or **Windows 11** (64-bit)

### 2. Visual Studio 2022

You have two options:

**Option A: Visual Studio 2022 Community (Recommended for most users)**
- Download from: https://visualstudio.microsoft.com/downloads/
- During installation, select the **"Desktop development with C++"** workload
- This includes:
  - MSVC compiler (v143 toolchain)
  - Windows SDK
  - CMake (3.22 or later)
  - Ninja build system

**Option B: Build Tools for Visual Studio 2022 (Minimal installation)**
- Download from: https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022
- Select:
  - **MSVC v143 - VS 2022 C++ x64/x86 build tools**
  - **Windows SDK (latest version)**
  - **CMake** (3.22 or later)
  - **Ninja** (optional, for faster builds)

### 3. Git for Windows
- Download from: https://git-scm.com/download/win
- Use default installation options
- Git Bash is optional (we'll use Command Prompt or PowerShell)

### 4. JUCE (Handled Automatically)

**No manual JUCE installation required!**

Zenith uses CMake's `FetchContent` to automatically download and configure JUCE 8 during the build process. This ensures everyone uses the exact same JUCE version with zero manual setup.

---

## Installation Steps

### Step 1: Clone the Repository

Open **Command Prompt** or **PowerShell** and run:

```cmd
git clone https://github.com/DaddyMilkMan/zenith-core.git
cd zenith-core
```

### Step 2: Configure with CMake

Generate the Visual Studio solution files:

```cmd
cmake -B build -G "Visual Studio 17 2022"
```

**What this does:**
- `-B build` — Creates a `build/` directory for generated files
- `-G "Visual Studio 17 2022"` — Generates Visual Studio 2022 project files
- CMake will automatically fetch JUCE 8 from GitHub (first run takes ~2 minutes)

**Optional: Enable/Disable MMCSS Audio Priority**

Zenith uses Windows MMCSS (Multimedia Class Scheduler Service) to boost the audio thread priority for better real-time performance. This is enabled by default.

To disable it:
```cmd
cmake -B build -G "Visual Studio 17 2022" -DZENITH_ENABLE_MMCSS=OFF
```

### Step 3: Build the Project

**Debug Build (recommended for development):**

```cmd
cmake --build build --config Debug
```

**Release Build (optimized, for testing performance):**

```cmd
cmake --build build --config Release
```

**Build both configurations:**

```cmd
cmake --build build --config Debug
cmake --build build --config Release
```

**Faster parallel builds:**

Add `-j` to use multiple CPU cores:

```cmd
cmake --build build --config Debug -j
```

---

## Running Zenith

After building, the executable will be in:

- **Debug:** `build/Debug/Zenith.exe`
- **Release:** `build/Release/Zenith.exe`

**To launch:**

```cmd
build\Debug\Zenith.exe
```

Or double-click `Zenith.exe` in File Explorer.

### First Run Checklist

When you first launch Zenith:

1. **Transport Controls** — Test Play/Stop/Record buttons
2. **Track View** — Test scrolling and zooming
3. **Sidebar** — Switch between tabs
4. **BPM Entry** — Try setting tempo or using tap tempo
5. **HiDPI** — If you have multiple monitors with different scaling, check for proper rendering

---

## Troubleshooting

### "CMake is not recognized"

**Fix:** Add CMake to your PATH:
1. Open **Visual Studio Installer**
2. Click **Modify** on your VS 2022 installation
3. Go to **Individual Components**
4. Search for "CMake" and ensure it's checked
5. Click **Modify** to install

Or download standalone CMake from https://cmake.org/download/

### "MSVC compiler not found"

**Fix:** Ensure you installed the **"Desktop development with C++"** workload in Visual Studio 2022.

### "fatal error C1083: Cannot open include file"

**Fix:** Let CMake finish fetching JUCE on first configure. If interrupted, delete the `build/` directory and run `cmake -B build -G "Visual Studio 17 2022"` again.

### Audio crackling or dropouts

**Check:**
1. Ensure MMCSS is enabled (default): `cmake -B build -G "Visual Studio 17 2022" -DZENITH_ENABLE_MMCSS=ON`
2. Rebuild: `cmake --build build --config Release -j`
3. Use **Release** build for performance testing (Debug builds are slower)
4. Close background apps (browsers, Discord, etc.)
5. Check audio buffer size in Zenith's audio settings (once implemented)

### Build hangs or takes forever

**Fix:**
- Use parallel builds: `cmake --build build --config Debug -j`
- Close other programs to free up RAM
- On first build, JUCE is fetched and compiled (~5-10 minutes)

---

## Clean Rebuild

If you encounter build errors after a `git pull` or branch switch:

```cmd
rmdir /s /q build
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug -j
```

This deletes the build directory and starts fresh.

---

## Development Workflow

See [DEVELOPER_WORKFLOW.md](DEVELOPER_WORKFLOW.md) for:
- Using Visual Studio IDE for debugging
- CMake GUI workflow
- Recommended VS Code / CLion setups
- Testing and profiling tools

---

## Next Steps

- **Read the Architecture Docs:** [docs/README.md](README.md)
- **Explore MMCSS Audio Priority:** [docs/windows/mmcss_audio_priority.md](windows/mmcss_audio_priority.md)
- **Windows Audio APIs Guide:** [docs/WINDOWS_AUDIO_APIS_GUIDE.md](WINDOWS_AUDIO_APIS_GUIDE.md)
- **Contributing:** See main [README.md](../README.md#contributing)

---

## Common CMake Options

```cmd
# Enable MMCSS audio thread priority boost (default: ON)
-DZENITH_ENABLE_MMCSS=ON

# Enable Link-Time Code Generation for Release builds (default: OFF)
-DZENITH_LTCG=ON

# Treat warnings as errors in CI builds (default: OFF locally)
-DZENITH_WERROR_CI=ON
```

Example with multiple options:

```cmd
cmake -B build -G "Visual Studio 17 2022" -DZENITH_ENABLE_MMCSS=ON -DZENITH_LTCG=ON
```

---

## Why No WSL / MSYS2 / vcpkg?

**Zenith is a native Windows DAW.** We use:
- **Visual Studio's MSVC compiler** (industry standard for Windows audio apps)
- **CMake FetchContent** for dependencies (JUCE is fetched automatically)
- **Native Windows APIs** (WASAPI, ASIO, MMCSS)

This ensures:
- Maximum performance
- Compatibility with all Windows audio drivers
- Simpler builds (no cross-compilation or compatibility layers)
- Industry-standard toolchain used by professional audio software

---

**Need help?** Open an issue at: https://github.com/DaddyMilkMan/zenith-core/issues
