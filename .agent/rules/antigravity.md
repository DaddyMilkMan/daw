# Anti-Gravity Agent Rules

**Mission:** Ensure Zenith DAW floats effortlessly (high performance) and doesn't crash (stability).

## 1. Zero-Gravity Audio (Real-Time Safety)
- **NO Allocations:** Heap allocations (`new`, `malloc`, `std::vector::resize`, `std::string` creation) are STRICTLY PROHIBITED in the audio thread (`processBlock`, `getNextAudioBlock`).
- **NO Locks:** Mutexes and critical sections are PROHIBITED in the audio thread. Use `std::atomic`, `juce::AbstractFifo`, or RCU patterns.
- **NO Blocking:** File I/O, networking, and `sleep()` calls are PROHIBITED in the audio thread.

## 2. Warp-Speed Rendering (Skia Stability)
- **Release Only:** Skia rendering MUST be built in **Release** mode.
    - *Reason:* Skia DLLs/ABIs often conflict with Debug runtime libraries, causing instability, crashes, or "weird" rendering artifacts.
    - *Enforcement:* The build system must prevent compiling Skia in Debug mode.
- **Always On:** Skia rendering MUST NOT be disabled except for specific low-level debugging of the fallback renderer.
    - *Launch Rule:* Any "Launch DAW" command or script MUST ensure the application runs with Skia enabled.
- **60 FPS Target:** All UI components must render efficiently enough to maintain 60 FPS. Avoid expensive paint calls; cache paths and gradients.

## 3. Telepathic UI (Responsiveness)
- **Async I/O:** All File I/O and Networking initiated by the UI MUST be asynchronous.
    - Never block the Message Thread.
- **No Sleep:** `juce::Thread::sleep()` is PROHIBITED on the Message Thread.

## 4. Code Hygiene
- **No Stubs:** All committed code must be implemented. No "TODO: implement this later" for critical features.
- **No Junk:** No `_Fixed.cpp`, `.bak`, or commented-out blocks of legacy code.
