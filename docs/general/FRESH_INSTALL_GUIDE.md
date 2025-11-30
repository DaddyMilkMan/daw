# Zenith DAW - Fresh Install Guide

This guide will take you from a completely fresh Windows PC to a working build of Zenith DAW with the Skia UI.

## 1. Install Prerequisites

### A. Visual Studio 2022 (Community)
1.  Download **Visual Studio 2022 Community** from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/vs/community/).
2.  Run the installer.
3.  Select the **"Desktop development with C++"** workload.
4.  Ensure the following optional components are checked (usually default):
    *   MSVC v143 - VS 2022 C++ x64/x86 build tools
    *   Windows 11 SDK (or Windows 10 SDK)
    *   C++ CMake tools for Windows
5.  Install and restart if prompted.

### B. Git
1.  Download **Git for Windows** from [git-scm.com](https://git-scm.com/download/win).
2.  Install with default settings.
3.  Open a command prompt (`cmd`) and verify:
    ```cmd
    git --version
    ```

### C. CMake (Optional but recommended)
*Visual Studio includes CMake, but a standalone installation is often easier to use from the command line.*
1.  Download the latest **CMake** installer from [cmake.org](https://cmake.org/download/).
2.  During installation, select **"Add CMake to the system PATH for all users"**.
3.  Verify:
    ```cmd
    cmake --version
    ```

### D. Ninja (Recommended for speed)
1.  Download **Ninja** binary from [github.com/ninja-build/ninja/releases](https://github.com/ninja-build/ninja/releases).
2.  Place `ninja.exe` in a folder in your PATH (e.g., `C:\Windows\System32` or a custom tools folder).
3.  Verify:
    ```cmd
    ninja --version
    ```

---

## 2. Install Skia via vcpkg

This is the most critical step for the custom UI.

1.  **Open Command Prompt (cmd) as Administrator** (recommended for first setup).
2.  **Clone and bootstrap vcpkg:**
    ```cmd
    cd C:\
    git clone https://github.com/microsoft/vcpkg
    cd vcpkg
    bootstrap-vcpkg.bat
    ```
3.  **Install Skia:**
    *This will take 5-15 minutes as it compiles Skia from source.*
    ```cmd
    vcpkg install skia:x64-windows
    ```
4.  **Integrate with Visual Studio (Optional but helpful):**
    ```cmd
    vcpkg integrate install
    ```

---

## 3. Clone and Build Zenith DAW

1.  **Clone the repository:**
    ```cmd
    cd C:\
    git clone https://github.com/YourUsername/zenith-daw.git
    cd zenith-daw
    ```
    *(Replace URL with your actual repo URL)*

2.  **Build with Skia Enabled:**
    The project includes a helper script that handles the CMake configuration for you.
    ```cmd
    BUILD_WITH_SKIA.bat
    ```

    **What this script does:**
    *   Sets up the Visual Studio environment.
    *   Configures CMake with `-DZENITH_ENABLE_SKIA=ON`.
    *   Points CMake to your vcpkg installation (`C:/vcpkg/scripts/buildsystems/vcpkg.cmake`).
    *   Builds the project using Ninja.

### Troubleshooting Build Issues

*   **"Could not find toolchain file"**: Ensure vcpkg is at `C:\vcpkg`. If you installed it elsewhere, edit `BUILD_WITH_SKIA.bat` to point to the correct path.
*   **"Skia not found"**: Ensure you ran `vcpkg install skia:x64-windows` successfully.
*   **JUCE errors**: JUCE is downloaded automatically. If you have internet connection issues, the download might fail. Try running the build script again.

---

## 4. Running the DAW

After a successful build, the executable will be located at:
`C:\zenith-daw\build\modules\zenith-core\ZenithDAW_artefacts\Debug\ZenithDAW.exe`

(Or `Release` if you built in release mode).
