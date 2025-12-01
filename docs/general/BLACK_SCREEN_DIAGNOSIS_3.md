# Emergency Team Meeting: Black Screen Diagnosis (Session 3)
**Date:** November 30, 2025
**Incident:** "Black Screen" persists. `debug_log.txt` was NOT created, indicating early failure.

## New Findings
*   **User Report:** "Black screen."
*   **Telemetry:** `debug_log.txt` does not exist.
*   **Implication:** The application is failing **before** `MainComponent` constructor starts.

## Team Analysis

**Alex (Systems Architect):**
"If `MainComponent` isn't constructing, we are crashing in `MainWindow` constructor or earlier. `MainWindow` creates `Engine` before `MainComponent`. If `Engine` initialization hangs or crashes (e.g., audio device init), we never get to the UI."

**Jenkins (Build Engineer):**
"Or it could be the `ZenithDAW` application class itself. We need to find the entry point."

**Dr. Log:**
"We need to move the logging 'upstream'. I want to instrument `ZenithDAW.cpp` (the `JUCEApplication`) and the `MainWindow` constructor."

## Action Plan: "Trace the Birth"

1.  **Locate Entry Point:** Find `ZenithDAW.cpp` (or equivalent `JUCEApplication` subclass).
2.  **Instrument Entry Point:** Add `logToDisk` to `initialise()`.
3.  **Instrument MainWindow:** Add `logToDisk` to `MainWindow` constructor *before* `Engine` creation.
4.  **Instrument Engine:** Add `logToDisk` to `Engine` constructor.

## Goal
Find the "Last Known Survivor" line of code.
