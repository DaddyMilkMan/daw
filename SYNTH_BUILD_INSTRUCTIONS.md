# Zenith PolySynth Build Instructions

## Prerequisites
*   CMake 3.15+
*   Visual Studio 2019 or later (with C++ desktop development workload)
*   Vcpkg (integrated with CMake)

## Status
**Sprint 3 Complete:**
*   Modulation Matrix (Neural Grid)
*   Real-time Visualizer
*   Filter 2 Controls
*   Learning Mode (Tooltips)

## Build Steps

1.  **Configure:**
    ```powershell
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
    ```

2.  **Build:**
    ```powershell
    cmake --build build --config Release
    ```

3.  **Run:**
    The standalone executable will be in `build/ZenithPolySynth_artefacts/Release/Zenith Poly Synth.exe`.
