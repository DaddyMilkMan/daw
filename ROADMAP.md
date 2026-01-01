# 🤖 Zenith DAW: "Titanium Scale" Master Protocol

## 🛰️ THE INVARIANT CONTRACT
**Preamble for ALL AI Prompts:**
1. **Audio Thread:** Zero allocation, zero locks, zero logging. 
2. **UI Thread:** Skia-only rendering. No `juce::Graphics`. GL calls strictly on GL thread.
3. **State:** `juce::ValueTree` is the single source of truth. 
4. **Wayland:** Handle Context/FBO loss per-frame in `renderFrame()`.
5. **No Stubs:** Provide full, production-ready implementation.

---

## 📅 THE EXECUTION PLAN (2026)

### JANUARY: VISUAL ENGINE
**Keystone:** `SkiaRenderer.cpp` (The GL/Skia Handshake)
- **Week 1 (HARD):** FBO Wrapping, Context Resiliency, EGL Bindings.
- **Week 2 (SOFT):** `ZenithTheme.h`, NeonGlow, SkiaButton widgets.
- **Week 3 (HARD):** `RenderTree.cpp` with `SkPicture` layer caching.
- **Week 4 (KILL SWITCH):** UI Cleanup. Delete all redundant skeleton code.

### FEBRUARY: DATA & AUDIO
**Keystone:** `ProjectState.cpp` (The ValueTree Logic)
- **Week 1 (HARD):** `ValueTree` DNA, Schema constants, XML serialization.
- **Week 2 (SOFT):** SPSC Metering Bridge (Audio -> UI visuals).
- **Week 3 (HARD):** PipeWire/JACK engine integration. Zero-lock callback.
- **Week 4 (GOLDEN PATH):** Milestone: Slider -> ValueTree -> Sine Volume -> Meter.

### MARCH: THE ARRANGER
**Keystone:** `TimelineModel.cpp` (The Math)
- **Week 1 (HARD):** Sample-accurate timeline math + 1/16th Snapping.
- **Week 2 (SOFT):** `WaveformCache.cpp` + Skia-path waveform rendering.
- **Week 3 (HARD):** Background WAV recording thread.
- **Week 4 (STRESS):** Performance audit (200 tracks + rapid resize).

### APRIL: AI BRAIN
**Keystone:** `ActionExecutor.cpp` (The AI-to-C++ Bridge)
- **Week 1 (HARD):** Non-blocking Grok 4.1 API client.
- **Week 2 (SOFT):** Glassmorphic Chat UI.
- **Week 3 (HARD):** Action Parser (JSON to ValueTree operations).
- **Week 4 (SUBTRACTIVE):** Bug squash and API freeze.

---

## 🚀 STARTING PROMPT (Jan Week 1)
> "I am building the `SkiaRenderer` keystone for Zenith DAW.
> Environment: Pop!_OS Wayland.
> Mandates: Option A+, JUCE owns GL thread, wrap FBO every frame, detect resize/DPI mismatch per-frame.
> Task: Implement `SkiaRenderer.h` and `.cpp` initialization logic. No stubs."
