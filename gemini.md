# Zenith DAW - Development Context & Roadmap

## 🛑 Core Mandates (Read First)
1.  **No Shortcuts:** Implement features properly the first time. Do not hack together "temporary" solutions unless absolutely necessary for a specific, isolated test. If a feature is complex, break it down, but do not skip essential logic (e.g., error handling, thread safety).
2.  **No Stubs:** "Placeholder" code is unacceptable for committed features. If a UI component is added, it must work. If an audio processor is added, it must process audio. Do not leave empty `void func() {}` blocks or comments like `// TODO: Implement this` in critical paths.
3.  **Flecs (ECS) is ABANDONED:** Do not attempt to re-integrate Flecs. The architecture is firmly based on `ProjectState` (ValueTree) and `Engine`.

## 🚀 Current Status: "Neon Noir" Skia UI & Customization
The UI has been fully overhauled to use Skia for rendering (hardware accelerated) with a "Neon Noir" aesthetic.
A comprehensive Settings Pane has been added, including Theme and Layout customization.

### ✅ Completed Features
1.  **Graphics Backend:** Native D3D12/Metal/Vulkan implementation with "Auto" detection.
2.  **UI Rendering:** All major components (`PianoRoll`, `Mixer`, `Arranger`, `Transport`) are now `SkiaComponent` based.
3.  **Visual Style:** Dark glassmorphism, neon accents, and a global `Glow Intensity` setting.
4.  **Customization:** `ThemeManager` (JSON themes) and `LayoutManager` (JSON layouts) infrastructure.
5.  **Audio Engine:** C++20 Lock-free threading fixes applied.

### ❌ Cancelled / Abandoned
*   **Flecs (ECS) Integration:** The migration to the Entity Component System (Flecs) for track/clip state has been **cancelled**. The project will continue using the existing `ProjectState` (ValueTree) and `Engine` class hierarchy. Do not attempt to re-integrate Flecs.

### ⚠️ Pending Implementation (File Lock Issue) - RESOLVED
The **"Edit Mode" (Drag-and-Drop UI)** file lock issue on `MainWindow.h` and `MainWindow.cpp` has been **resolved**.
*   `MainWindow.cpp` updated to include `skia/ZenithDesignSystem.h`.
*   Drag logic verified in `mouseDrag`.

### ❌ Build Failure (D8016)
The build fails with `cl : command line error D8016: '/Gd' and '/Gr' command-line options are incompatible`.
*   **Investigation:** `/Gr` (FastCall) and `/Gd` (Cdecl) are both being passed to the compiler.
*   **Actions Taken:**
    *   Removed explicit `/Gd` from `CMakeLists.txt`.
    *   Added logic to strip `/Gr` and `/Gd` from `CMAKE_CXX_FLAGS`.
    *   Attempted to force `FastCall`, `Cdecl`, and `VectorCall` in `ZenithDAW.vcxproj`.
    *   Disabled `ccache`.
    *   Disabled `ENABLE_IPO` and `ENABLE_HARDENING`.
*   **Result:** Error persists. The source of the conflicting `/Gr` flag remains unidentified. It is NOT in the vcxproj file text, suggesting it is injected by a toolchain default or a dependency configuration not visible in standard logs.

### 🛠️ Next Steps
1.  **Resolve D8016:** Continue investigation. Consider `JuceLibraryCode` or other CMake generated files.
2.  **Test Customization:** Once built, Open Settings -> Appearance. Toggle "Edit UI Mode". Try dragging the Transport Bar or Side Panel.
3.  **Audio Test:** Verify audio engine stability with the new lock-free `TrackSnapshot`.

---
*Last Updated: 2025-12-03*