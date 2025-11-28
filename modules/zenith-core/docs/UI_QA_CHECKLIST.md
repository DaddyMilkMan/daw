# UI/UX Quality Assurance Checklist
**Last Updated:** 2025-11-23
**Status:** Draft - Not Yet Validated

> **IMPORTANT:** This checklist represents work that is *structurally complete* but **NOT production-validated**. All items below must pass before calling the UI "production-ready".

## 🔴 CRITICAL: What "Production-Ready" Actually Means

The UI code is:
- ✅ Structurally sound (compiles, uses proper patterns)
- ✅ Documented (session summary, inline comments)
- ⚠️ **NOT visually QA'd** (no human has seen it running)
- ⚠️ **NOT performance tested** (no stress testing with real sessions)
- ⚠️ **NOT RT-safety verified** (no grep audit for locks/allocations)

---

## 1. Visual QA - Metering System

### Basic Functionality
- [ ] **Meters respond to audio playback**
  - Play a sine wave at -12 dBFS
  - Meters should show activity in green/yellow zone
  - Peak hold indicators should appear and fade correctly

- [ ] **Color zones make sense**
  - At typical mix levels (-18 to -12 dBFS), meters should be green/yellow
  - Red zone should only appear near 0 dBFS (digital clipping)
  - NOT screaming red at normal operating levels

- [ ] **Ballistics feel natural**
  - Attack feels responsive (not laggy)
  - Release feels smooth (not jittery)
  - Peak hold duration feels right (~1.5 seconds)

- [ ] **Edge cases**
  - Meters at -∞ (silence): show zero, no artifacts
  - Meters with master muted: correctly reflect muted state
  - Meters with very short transients: peak hold captures them

### Visual Polish
- [ ] **No visual glitches**
  - No flickering at 60 FPS
  - No tearing or artifacts
  - Smooth animation throughout range

- [ ] **Theme integration**
  - Colors match SkiaTheme (meterGreen, meterYellow, meterRed)
  - Background/border colors consistent with rest of UI
  - Typography matches theme (small size for labels)

---

## 2. Visual QA - Automation Lanes

### Basic Interaction
- [ ] **Control point manipulation**
  - Click empty space: adds point at correct time/value
  - Drag point: moves smoothly, snaps to grid if enabled
  - Double-click point: deletes point correctly

- [ ] **Visual feedback**
  - Hover over point: shows glow effect
  - Select point: shows selection tint + border
  - Dragging point: visual state clearly indicates "being dragged"

### Tooltips
- [ ] **Hover tooltips work correctly**
  - Hover over automation point: tooltip appears immediately
  - Tooltip shows correct value + time (e.g., "75% @ 8.50 beats")
  - Tooltip positioned to avoid screen edges
  - Tooltip disappears on mouse exit

### Curve Rendering
- [ ] **Automation curves look smooth**
  - No jagged edges (anti-aliasing works)
  - Curves extend correctly from left to right edge
  - Multiple points render correctly in sorted order

- [ ] **Theme integration**
  - Curve color matches SkiaTheme.accentMain (teal)
  - Selected points use accentAlt (blue)
  - Background uses bg1 with subtle tint

---

## 3. Visual QA - Selection States

### Clarity of Selection
- [ ] **Track selection is obvious**
  - Selected track clearly visually distinct from unselected
  - Border + tint renders correctly (BorderAndTint mode)

- [ ] **Clip selection is obvious**
  - Selected clip clearly highlighted
  - Can visually distinguish: track selected + clip unselected vs both selected

- [ ] **Note selection (piano roll)**
  - Selected notes use SelectionStyle consistently
  - Multi-select shows all selected notes clearly

- [ ] **Automation point selection**
  - Selected point shows blue border + tint background
  - Radius expansion (+ 4px) is visible but not excessive

### Focus States
- [ ] **Keyboard focus is clear**
  - Focus ring (focusBorder) appears on focused element
  - Tab navigation shows clear focus indicator
  - Focus doesn't get "lost" in the UI

---

## 4. Performance Sanity Checks

### Baseline Performance
- [ ] **Idle CPU usage acceptable**
  - With no audio playing, UI updates don't spike CPU
  - Meters at 60 FPS don't cause excessive background load

- [ ] **No FPS drops during normal use**
  - Scrolling timeline: smooth 60 FPS
  - Zooming in/out: no jank or stuttering
  - Playing audio + meters updating: no frame drops

### Stress Test Scenarios
- [ ] **Busy session (24+ tracks)**
  - Full-screen window with many tracks visible
  - Each track has clips, automation lanes visible
  - Scroll + zoom: remains smooth
  - CPU usage stays reasonable (< 10% of one core for UI alone)

- [ ] **Many automation points (100+ per lane)**
  - Automation lane with dense point data
  - Zoom in/out: curve renders smoothly
  - Hover tooltips don't cause stuttering

- [ ] **Rapid meter updates**
  - Audio with fast transients (drums, percussion)
  - Meters update correctly without glitches
  - Peak hold doesn't miss short spikes

### Specific Concerns
- [ ] **Automation lane Skia surface creation**
  - Creating temp SkSurface on every paint() is NOT causing jank
  - Profile: is this a bottleneck? (Consider caching surface)

- [ ] **Meter timer at 60 FPS**
  - 16ms timer callback isn't causing audio thread issues
  - Check: does Timer run on message thread? (Should be yes)

---

## 5. RT Safety Verification

### Audio Thread Purity
- [ ] **No new allocations in audio path**
  ```bash
  grep -r "new\|delete\|malloc\|free" include/ src/ | grep -i "process\|audio\|render"
  ```

- [ ] **No locks in audio callbacks**
  ```bash
  grep -r "std::mutex\|CriticalSection\|lock\|SpinLock" include/ src/ | grep -i "process\|audio\|render"
  ```

- [ ] **Meter level setting is lock-free**
  - `setLevel()` and `setPeakLevel()` are simple float assignments
  - Called from audio thread → verify no hidden allocations
  - Float assignment on x64 is atomic, but double-check on ARM

### Message Thread Discipline
- [ ] **All UI updates on message thread**
  - Meter `timerCallback()` runs on message thread
  - Automation `repaint()` calls are message-thread only
  - No UI manipulation from audio thread

---

## 6. Workflow Bug Hunt

> **DO THIS:** Use the DAW for 30–60 minutes like a real user. Note everything that feels "off".

### Metering Workflow
- [ ] Load a project with drums/bass/synths
- [ ] Play back, watch meters
- [ ] Note any issues:
  - Meters too sensitive / not sensitive enough?
  - Colors make sense for your typical mix levels?
  - Peak hold timing feels right?

### Automation Workflow
- [ ] Create a track, add automation lane for volume
- [ ] Add 10–15 automation points
- [ ] Edit them: move, delete, add more
- [ ] Note any issues:
  - Hit zones too small / too large?
  - Tooltips helpful or annoying?
  - Curve rendering clear or confusing?
  - Grid snapping works as expected?

### Selection Workflow
- [ ] Select tracks, clips, notes, automation points
- [ ] Multi-select with Shift/Ctrl
- [ ] Note any issues:
  - Selection hard to see?
  - Confused which element is selected?
  - Focus states unclear?

### Keyboard/Mouse Workflow
- [ ] Tab through focusable elements
- [ ] Use modifier keys (Shift, Ctrl, Alt) for selections
- [ ] Mouse wheel for zoom/scroll
- [ ] Note any issues:
  - Unexpected behavior with modifiers?
  - Scroll direction wrong?
  - Wheel zoom in wrong direction?

---

## 7. Debug HUD Checklist

### Add Debug Overlay Toggle
- [ ] **Global key (e.g., Ctrl+Shift+D) to toggle debug HUD**
- [ ] **Debug HUD shows:**
  - FPS estimate (average over last 60 frames)
  - Frame time (ms per frame)
  - Layout bounds overlay (optional: show component rectangles)
  - Mouse position + hover state

- [ ] **Debug HUD styling:**
  - Uses Typography.tiny for text
  - Semi-transparent bg3 background
  - Positioned in top-right corner
  - Doesn't interfere with normal workflow

### Debug Logging
- [ ] **Conditional logging for development**
  ```cpp
  #ifdef ZENITH_DEBUG
      DBG("Meter level: L=" << leftLevel_ << " R=" << rightLevel_);
  #endif
  ```

- [ ] **Removed before production**
  - No debug prints in release build
  - No leftover test code

---

## 8. Code Quality Final Pass

### Clean Code
- [ ] **No commented-out code**
- [ ] **No TODO/FIXME comments without tracking**
- [ ] **Consistent naming conventions**
- [ ] **All public APIs documented**

### Test Coverage
- [ ] **Unit tests for meter ballistics**
  - Test attack/release factors
  - Test peak hold timing
  - Test color zone thresholds

- [ ] **Integration tests for automation**
  - Test coordinate conversion (beats ↔ pixels)
  - Test grid snapping logic
  - Test point hit detection

### Thread Safety
- [ ] **Document thread ownership**
  - Which methods are audio-thread safe?
  - Which require message thread?
  - Add assertions to enforce this

---

## Acceptance Criteria

Before merging to main / calling this "production-ready":

**ALL items in sections 1-6 must be checked ✓**

**At least 80% of section 7 (debug HUD) should be implemented**

**At least 50% of section 8 (code quality) should pass**

**Workflow bug hunt (section 6) should have ZERO critical issues**

---

## Current Status

**Date:** 2025-11-23
**Checklist Completion:** 0% (not yet run)

**Blocking Issues:** None known (but none tested either!)

**Next Actions:**
1. Build succeeds for UI targets (currently blocked by test failures)
2. Launch DAW, verify meters + automation render
3. Run through sections 1-3 (visual QA)
4. Profile performance (section 4)
5. Verify RT safety (section 5)
6. Do 30-min workflow test (section 6)

---

## Notes

This checklist is intentionally strict. "Production-ready" means **users can rely on it**, not just "it compiles and looks cool in theory".

If you find issues during QA, **document them here** with severity:
- 🔴 **Critical:** Blocks production use (crashes, data loss, audio glitches)
- 🟡 **Major:** Degrades experience (visual bugs, performance issues)
- 🟢 **Minor:** Polish issues (tooltips slightly off, etc.)

Then come back and create a focused "fix UX paper cuts" session to address them.
