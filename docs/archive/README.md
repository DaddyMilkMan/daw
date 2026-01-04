# Source Directory Structure (Citadel)

This directory follows the **Citadel** architectural pattern.
**DO NOT ADD FILES TO THE ROOT OF THIS DIRECTORY.**

## Organization Rules

1.  **`ui/`**: All User Interface components.
    *   *Examples*: `MainWindow.cpp`, `ArrangerComponent.cpp`, `PianoRollEditor.cpp`
    *   *Rule*: If it inherits from `juce::Component`, it goes here.

2.  **`engine/`**: Core Audio Engine logic.
    *   *Examples*: `Engine.cpp`, `ProjectState.cpp`, `Track.cpp`, `Clip.cpp`
    *   *Rule*: If it processes audio or manages state, it goes here.

3.  **`instruments/`**: Built-in Instruments.
    *   *Examples*: `ZenithPolySynth.cpp`, `ZenithSampler.cpp`

4.  **`commands/`**: Command Pattern & API.
    *   *Examples*: `CommandAPI.cpp`, `SessionGraph.cpp`

5.  **`network/`**: Networking & AI Bridge.
    *   *Examples*: `AIBridgeClient.cpp`, `GrokAPIClient.cpp`

6.  **`rendering/`**: Graphics Context Management.
    *   *Examples*: `SkiaContextManager.cpp`

## Naming Conventions
*   **Files**: PascalCase (e.g., `MyComponent.cpp`)
*   **Classes**: PascalCase (e.g., `class MyComponent`)
*   **Headers**: Must be located in `../../include/` or `../../include/ui/`.

## Prevention of "Split Brain"
*   **Check before creating**: Ensure a component doesn't already exist in another folder.
*   **No duplicates**: Do not create `ArrangementComponent` if `ArrangerComponent` exists.
