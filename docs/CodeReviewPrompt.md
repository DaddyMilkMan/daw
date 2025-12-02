# Zenith DAW - Code Review & Feedback Prompt

**Objective:** Evaluate the current state of the Zenith DAW "ZenithPolySynth" implementation, specifically focusing on the new Skia-based UI and the underlying DSP/Architecture.

**Context:** We have just integrated a Skia-based UI (`ZenithPolySynthUI`, `ZenithUIComponents`) to replace the standard JUCE Graphics. The goal is a "Neon Noir / Glassmorphism" aesthetic with high performance. We have also implemented a new preset system and "Learning Mode".

**Instructions for Reviewers:**
Please review the codebase from your specific persona's perspective. Identify strengths, weaknesses, bugs, and areas for improvement. Be critical but constructive.

---

## 1. Producer: "The Hitmaker" (EDM/Pop)
*   **Focus:** Workflow speed, visual clarity, "vibe".
*   **Questions:**
    *   Does the UI look inspiring? Does it make you want to make music?
    *   Are the main controls (Cutoff, Res, Envelopes) big and easy to grab?
    *   Is the "Neon" look too distracting or just right?
    *   Does the "Learning Mode" help you understand the synth quickly?

## 2. Producer: "The Composer" (Film/Orchestral)
*   **Focus:** Modulation depth, preset management, reliability.
*   **Questions:**
    *   Is the Preset Browser easy to navigate?
    *   Can you see modulation routings clearly?
    *   Does the UI feel stable and professional?
    *   Is the text legible?

## 3. Producer: "The Sound Architect" (Sound Design)
*   **Focus:** Deep editing, modulation matrix, visualizers.
*   **Questions:**
    *   Does the Visualizer give accurate feedback on the sound?
    *   Is the Modulation Matrix (placeholder) positioned logically?
    *   Are the LFO and Envelope controls precise enough?

## 4. Producer: "The Performer" (Live)
*   **Focus:** CPU usage, stability, visibility on stage.
*   **Questions:**
    *   Is the UI high-contrast enough for a dark stage?
    *   Does the UI lag? (FPS check)
    *   Are the controls large enough for touch screens?

## 5. Coder: "Dr. DSP" (Audio Processing)
*   **Focus:** Audio quality, filter stability, aliasing.
*   **Questions:**
    *   Review `ZenithPolySynthProcessor` parameter handling.
    *   Are we smoothing parameters correctly to avoid zippers?
    *   Is the visualizer thread-safe?

## 6. Coder: "Pixel Perfect" (UI/UX)
*   **Focus:** Skia implementation, rendering performance, layout.
*   **Questions:**
    *   Review `ZenithUIComponents.h` and `ZenithPolySynthUI.cpp`.
    *   Are we using `SkCanvas` efficiently (save/restore, layers)?
    *   Is the "Glassmorphism" effect implemented performantly (blur usage)?
    *   Are the custom components (`ZenithKnob`, `ZenithSlider`) reusable and clean?

## 7. Coder: "The Architect" (Systems)
*   **Focus:** Code structure, memory management, threading.
*   **Questions:**
    *   Review `ZenithPolySynthUI` lifecycle and OpenGL context management.
    *   Are we leaking any Skia resources?
    *   Is the `ZenithPresetManager` singleton safe?
    *   Are we using `std::unique_ptr` correctly for components?

## 8. Coder: "The Builder" (Integrity)
*   **Focus:** Build system, dependencies, warnings.
*   **Questions:**
    *   Review `CMakeLists.txt` changes for Skia integration.
    *   Are the include paths (`<include/core/...>`) correct and portable?
    *   Are there any remaining compiler warnings?
    *   Does the vcpkg integration look robust?

---

**Output Format:**
Please provide your feedback in a bulleted list under your Persona Header.
