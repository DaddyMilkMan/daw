# 🤖 AGENT PROMPT: Zenith DAW "Alternatives" Implementation
**Objective:** Implement critical missing DAW components (Browser, Master Section, Routing Matrix) with uncompromising quality and completeness.

## 🛑 CORE PHILOSOPHY: The Principle of Non-Termination and Thoroughness
**CRITICAL:** You are opting into a "No Stub" workflow. Speed is irrelevant. Quality and completeness are everything.

1.  **NO STUBS:** Never write `// TODO: implement later` or create empty placeholder methods. If a feature is required, implement it fully.
2.  **NO SHORTCUTS:** Do not mock data unless absolutely necessary for testing. Connect to the real `Engine`, `ProjectState`, and `FileSystem`.
3.  **VISUAL EXCELLENCE:** Use the Skia-based "Neon Noir" design system. Components must look professional, polished, and animated.
4.  **VERIFICATION:** You must compile and verify every step.

---

## 🛠️ TASKS

### 1. 📂 Browser Component
**Goal:** A full-featured file and asset browser.
**Requirements:**
*   **File System Navigation:** Real directory traversal `c:\`.
*   **Audio Preview:** clicking a file must play it (use `Engine` or a temporary `AudioTransportSource`).
*   **Waveform Preview:** Show a mini-waveform of the selected sample.
*   **Drag-and-Drop:** thoroughly implement `ExternalDragGesture` to allow dragging files onto the `Arranger` tracks.
*   **Search/Filter:** Real-time text filtering of file lists.
*   **Sample Database:** (Bonus) basic tagging system using a JSON index.

### 2. 🎚️ Master Section Component
**Goal:** A professional master bus control center.
**Requirements:**
*   **Master Fader:** Large, high-precision fader controlling `Engine::masterVolume`.
*   **Metering:** Real stereo peak meters AND LUFS momentary/short-term metering.
*   **Plugin Chain:** A vertical slot list showing plugins on the Master Bus.
    *   Click to open editor.
    *   Drag to reorder (real implementation).
    *   Bypass buttons.
*   **Output Routing:** Selector for hardware audio outputs (query `AudioDeviceManager`).

### 3. 🕸️ Routing Matrix Component
**Goal:** Visual signal flow management.
**Requirements:**
*   **Grid View:** Tracks on Y-axis (Sources), Outputs/Busses on X-axis (Destinations).
*   **Interaction:** Click grid points to toggle routing.
*   **Feedback:** Active routes light up.
*   **Aux Sends:** Visualize send levels (maybe as mini-knobs within the grid cells).
*   **Sidechains:** Explicit visualization of sidechain routing.

---

## 📝 IMPLEMENTATION GUIDELINES

### Tech Stack
*   **UI:** `SkiaComponent` (Hardware accelerated).
*   **State:** `ProjectState` (ValueTree) + `Engine` (Processors).
*   **Language:** C++20 (JUCE Framework).

### Workflow per Component:
1.  **Design types first:** Define struct/class data models.
2.  **Implement logic:** processing, data management, state syncing.
3.  **Implement UI:** Skia rendering, mouse handling.
4.  **Connect:** Hook up to `CommandAPI` and `ProjectState`.
5.  **Polish:** Add animations, tooltips, hover states.

**GO SLOW. DO IT RIGHT. MAKE IT REAL.**
