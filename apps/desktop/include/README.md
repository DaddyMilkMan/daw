# Include Directory Structure (Citadel)

This directory contains the public headers for the application.
It mirrors the structure of the `Source` directory where applicable.

## Organization Rules

1.  **`include/` (Root)**: Core Engine & System Headers.
    *   *Examples*: `Engine.h`, `ProjectState.h`, `Track.h`
    *   *Rationale*: These are the fundamental types used throughout the app.

2.  **`include/ui/`**: User Interface Headers.
    *   *Examples*: `ArrangerComponent.h`, `PianoRollEditor.h`, `MainWindow.h`
    *   *Rationale*: UI components should be grouped to avoid cluttering the root.

## Usage
*   **Engine Headers**: `#include "Engine.h"` (Add `include` to include path)
*   **UI Headers**: `#include "ui/ArrangerComponent.h"`

## Prevention of Duplicates
*   **Do not** create headers in `Source/`. All public headers must be here.
*   **Do not** duplicate headers (e.g., `ArrangementComponent.h` vs `ArrangerComponent.h`).
