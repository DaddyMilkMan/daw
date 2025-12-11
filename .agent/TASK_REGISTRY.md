# 🤖 Zenith DAW AI Task Registry

This registry tracks the active agents, their assigned branches, and their specific prompts/missions.

## 🏗️ Active Agents & Assignments

### 1. Agent: `Fixer` 🔧
*   **Branch:** `agent/fixer/critical-repairs`
*   **Mission:** Fix the critical engine bugs that prevent basic functionality (Exporting, DSP).
*   **Tasks:**
    *   [ ] **Export Engine**: Implement `Engine::renderOfflineBlock` and fix silence in export.
    *   [ ] **DSP Integrity**: Audit and fix hardcoded sample rates (e.g., `48000.0f`) in DSP code.
*   **Prompt:**
    ```text
    You are the Fixer Agent. Your goal is to resolve critical engine bugs.
    1. Check `Source/engine/ExportEngineImpl.cpp` - it currently renders silence. Implement `Engine::renderOfflineBlock` and wire it up.
    2. Search for hardcoded `48000.0` or `44100.0` in `Source/dsp` and replace with proper context sample rates.
    ```

### 2. Agent: `Virtuoso` 🎹
*   **Branch:** `agent/virtuoso/piano-roll-impl`
*   **Mission:** Make the Piano Roll functional (currently a stub).
*   **Tasks:**
    *   [ ] **Rendering**: Draw MIDI notes from `ProjectState` / `Clip`.
    *   [ ] **Interaction**: Implement note addition/deletion on mouse click.
    *   [ ] **Playback**: Ensure edits reflect in the Engine immediately.
*   **Prompt:**
    ```text
    You are the Virtuoso Agent. Your goal is to implement the Piano Roll Editor (`Source/PianoRollEditor.cpp`).
    1. Remove "Integration Stub" text.
    2. Implement `paint` to render MIDI notes from the associated Clip's data.
    3. Implement `mouseDown` to add notes to the Clip.
    ```

### 3. Agent: `Director` 🎬
*   **Branch:** `agent/director/arranger-polish`
*   **Mission:** Complete the Arranger View interactions.
*   **Tasks:**
    *   [ ] **Clip Dragging**: Fix/Finish clip dragging logic in `ArrangerView`.
    *   [ ] **Automation**: Enable basic automation editing interactions.
*   **Prompt:**
    ```text
    You are the Director Agent. Your goal is to polish the Arranger View (`Source/ArrangerView.cpp`).
    1. Complete the clip dragging implementation (move clips in time/tracks).
    2. Ensure visual feedback during drag is smooth (Skia).
    3. Verify interactions update the `ProjectState`.
    ```

### 4. Agent: `Architect` 📐
*   **Branch:** `agent/architect/spaghetti-cleanup`
*   **Mission:** structural Refactoring and Synchronization.
*   **Tasks:**
    *   [ ] **ClipSynchronizer**: Finish bidirectional sync between Engine and ProjectState.
    *   [ ] **ProjectState**: Evaluate splitting `ProjectState.cpp` if strictly necessary, or at least clean it up.
*   **Prompt:**
    ```text
    You are the Architect Agent. Your goal is structural integrity.
    1. Focus on `Source/ClipSynchronizer.cpp`.
    2. Implement the missing bidirectional sync between Engine and ProjectState.
    3. Ensure recording a clip in Engine updates the ProjectState model correctly.
    ```

## 🔄 Workflow Status

| Agent | Branch | Status | Last Update |
|-------|--------|--------|-------------|
| Fixer | `agent/fixer/critical-repairs` | ⏳ Pending | - |
| Virtuoso | `agent/virtuoso/piano-roll-impl` | ⏳ Pending | - |
| Director | `agent/director/arranger-polish` | ⏳ Pending | - |
| Architect | `agent/architect/spaghetti-cleanup` | ⏳ Pending | - |
