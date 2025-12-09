# Zenith DAW - AI Developer Guide

**Welcome, AI Agent.** This document describes the project structure, coding conventions, and architecture of the Zenith DAW to help you navigate and modify the codebase efficiently.

## 1. Project Structure

The repository is located at `C:\zenith\daw`.

### Core Directories
*   **`apps/desktop`**: The main C++ JUCE application.
    *   **`Source/`**: C++ source code.
        *   **`engine/`**: The audio engine core.
            *   `Engine.cpp`: Core lifecycle and audio callback.
            *   `EngineLifecycle.cpp`: Init/Shutdown/State Sync.
            *   `EngineTransport.cpp`: Play/Stop/Record logic.
            *   `EngineRecording.cpp`: Recording logic and baking.
            *   `EngineExport.cpp`: Offline rendering.
            *   `Track.cpp`, `Clip.cpp`, `MixerChannel.cpp`: Core entities.
        *   **`ui/`**: JUCE components.
            *   `skia/`: Custom Skia-based UI components (modern UI).
            *   `PianoRollComponent.cpp`: Core interaction logic for MIDI editor.
            *   `PianoRollRendering.cpp`: Skia drawing code for Piano Roll.
            *   `PianoRollAdvanced.cpp`: Algorithms (Quantize, Humanize, etc.).
            *   `MainWindow.cpp`: Main application window and layout.
    *   **`include/`**: Public headers.
*   **`services/`**: Microservices.
    *   **`auth/`**: Node.js/Express authentication service (MongoDB).
*   **`docs/`**: Documentation.
*   **`logs/`**: Build logs and debug output.
*   **`tools/`**: Utility scripts (Python).

## 2. Architecture

*   **Language**: C++20 (DAW), Node.js (Auth Service).
*   **Frameworks**: JUCE 8 (Audio/GUI), Skia (Custom Rendering), Express.js (Backend).
*   **State Management**: `ProjectState` (wraps `juce::ValueTree`) is the single source of truth. The UI listens to ValueTree changes. The Engine synchronizes with ValueTree via `TrackStateSynchronizer`.
*   **Audio Engine**:
    *   **PDC**: Plugin Delay Compensation is fully implemented in `Engine` and `Track`.
    *   **Lock-Free**: The audio thread uses lock-free patterns (FIFOs, atomics) to communicate with the message thread.
    *   **Graph**: `RoutingGraph` manages signal flow.

## 3. Coding Conventions

*   **Files**: Split large classes into logical files (e.g., `Class.cpp`, `ClassRendering.cpp`, `ClassLogic.cpp`).
*   **Stubs**: **DO NOT** leave empty stubs. If you add a function, implement a basic version or a log message explaining why it's empty.
*   **Safety**: Always checking pointers (`nullptr`) before access. Use `jassert` for invariants.
*   **Formatting**: Standard JUCE style (Allman braces, 4 spaces).

## 4. Key Files to Know

*   **`Engine.cpp`**: The heart of the audio processing.
*   **`PianoRollComponent.cpp`**: The complex MIDI editor.
*   **`TransportBar.cpp`**: The UI control center (Skia-based).
*   **`authController.js`**: Backend auth logic.

## 5. Recent Changes (Refactoring)

*   **Split Engine**: `Engine.cpp` was monolithic; it is now split into 5 files.
*   **Split PianoRoll**: `PianoRollComponent.cpp` was monolithic; it is now split into 3 files.
*   **Cleanup**: Logs moved to `logs/`, unused files deleted.
*   **Auth**: Added `services/auth` for JWT authentication.

Use this guide to orient yourself before making changes.
