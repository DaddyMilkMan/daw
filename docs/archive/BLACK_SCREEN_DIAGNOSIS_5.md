# Emergency Team Meeting: Black Screen Diagnosis (Session 5)
**Date:** November 30, 2025
**Incident:** Linker error `LNK2001: unresolved external symbol MainComponent::drawSkiaContent`.

## New Findings
*   **Build Failure:** The latest build failed with a linker error.
*   **Old Executable:** The user was running an old executable (likely the one with Skia OFF), which explains the "Dark Grey" screen (JUCE fallback).
*   **Root Cause:** `MainComponent::drawSkiaContent` is declared but the linker can't find the implementation, even though it appears to be in `MainWindow.cpp`.

## Team Analysis

**Alex (Systems Architect):**
"We have a classic 'Ghost Implementation'. `MainWindow.cpp` has `drawSkiaContent` inside `#ifdef ZENITH_USE_SKIA`. The build system *says* `ZENITH_USE_SKIA` is ON. But the linker says 'Nope'."

**Jenkins (Build Engineer):**
"Wait. `MainWindow.cpp` includes `MainWindow.h`. `MainWindow.h` includes `SkiaMainWindowIntegration.h`.
If `MainComponent` inherits from `SkiaMainWindowIntegration`, it MUST implement `drawSkiaContent` (pure virtual).
If the implementation in `MainWindow.cpp` is somehow skipped (e.g., macro mismatch), we get this error."

**Dr. Log:**
"Let's look at the implementation in `MainWindow.cpp` again.
```cpp
#ifdef ZENITH_USE_SKIA
void MainComponent::drawSkiaContent(SkCanvas* canvas) { ... }
#endif
```
If `ZENITH_USE_SKIA` is NOT defined in `MainWindow.cpp` (but IS defined in `SkiaMainWindowIntegration.h` or globally), then the implementation is compiled out, but the declaration remains (if the header sees the macro).
BUT, `ZENITH_USE_SKIA` is a global definition in CMake. It should be everywhere."

**Sarah (UI/UX):**
"What if the signature is wrong?
`SkiaMainWindowIntegration.h`: `virtual void drawSkiaContent(SkCanvas* canvas) = 0;`
`MainWindow.h`: `void drawSkiaContent(SkCanvas* canvas) override;`
`MainWindow.cpp`: `void MainComponent::drawSkiaContent(SkCanvas* canvas) { ... }`
This looks fine. Unless `SkCanvas` is forward declared differently?"

**Action Plan: "The Linker Exorcism"**

1.  **Verify Headers:** Check `SkiaMainWindowIntegration.h` and `MainWindow.h` for the exact signature.
2.  **Verify Macro:** Ensure `ZENITH_USE_SKIA` is actually defined in `MainWindow.cpp`. (We saw the log "ZENITH_USE_SKIA IS DEFINED", so it *was* defined in the previous successful build... wait. The previous successful build had Skia OFF? No, the log said "ZENITH_USE_SKIA IS DEFINED". So the macro IS there).
3.  **Check for "Inline" Trap:** Is `drawSkiaContent` defined inline in the class declaration in `MainWindow.h` by mistake?
4.  **Proposed Fix:** Move the implementation of `drawSkiaContent` *out* of the `#ifdef` block in `MainWindow.cpp` (but keep the body empty if Skia is off? No, `MainComponent` only inherits `SkiaMainWindowIntegration` if Skia is ON).

## New Test Protocol (Proposed by User)

**"The Traffic Light Test"**
Instead of just "Blue or Black", let's define clear visual states:
1.  **RED Screen:** OpenGL Init Failed (Software Fallback active, but Skia failed).
2.  **BLUE Screen:** Software Fallback active (Skia disabled intentionally).
3.  **GREEN Screen:** Skia Init Success + `drawSkiaContent` called (We will force a Green clear in `drawSkiaContent`).
4.  **BLACK Screen:** Window not painting / Occluded.
5.  **DARK GREY Screen:** JUCE Fallback (Skia Macro OFF).

**Next Step:** Fix the linker error so we can run the Traffic Light Test.
