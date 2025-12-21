---
description: Full UI/UX refactoring workflow - Weeks 1-4
---

# UI Refactoring Workflow - From "Neon Chaos" to Production DAW

## Overview
This workflow implements the complete UI/UX refactoring plan from `docs/UI_REFACTORING_PLAN.md`.

**Duration**: 4 weeks  
**Goal**: Thread-safe, performant, scalable DAW UI

---

## Week 1: Thread Safety (CRITICAL)

### Prerequisites
- [x] Read `docs/UI_REFACTORING_PLAN.md`
- [x] Read `docs/PHASE1_RENDER_TREE_IMPLEMENTATION.md`
- [x] `RenderTree.h` created
- [x] `SkiaKnob::captureRenderState()` implemented

### Tasks

#### 1.1: Implement Frame Capture
**File**: `apps/desktop/Source/ui/skia/ZenithPolySynthUI.cpp`

Add the following methods (see `PHASE1_RENDER_TREE_IMPLEMENTATION.md` for full code):
- `timerCallback()` - Start frame capture
- `captureFrameSnapshot()` - Snapshot all UI state
- `drawKnobFromState()` - Draw knob from snapshot

```bash
# Verify implementation compiles
//  turbo
cmake --build build --config Release --target ZenithDAW
```

#### 1.2: Add Thread Safety Assertions
Add debug assertions to verify thread separation:
```cpp
jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
```

// turbo
```bash
cmake --build build --config Debug --target ZenithDAW
```

#### 1.3: Test Thread Safety
Run the DAW and verify:
- [ ] No crashes when opening menus during rendering
- [ ] No crashes when changing presets rapidly
- [ ] UI responds smoothly

Optional: Run with ThreadSanitizer if available:
```bash
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" -DCMAKE_BUILD_TYPE=Debug -S . -B build-tsan
cmake --build build-tsan
./build-tsan/apps/desktop/ZenithDAW.exe
```

#### 1.4: Extend to Other Controls
Add `captureRenderState()` to:
- [ ] `SkiaSlider` (or `ZenithSlider`)
- [ ] `ZenithButton`
- [ ] `ZenithVisualizer`
- [ ] `ZenithPresetBar`

// turbo
```bash
cmake --build build --config Release --target ZenithDAW
```

### Week 1 Deliverables
- [x] `RenderTree.h` with triple-buffer implementation
- [ ] Full frame snapshot in `captureFrameSnapshot()`
- [ ] Render thread draws from snapshot only (no Component access)
- [ ] Zero crashes during UI changes
- [ ] Debug assertions verify thread safety

---

## Success Metrics

**Before Refactoring**:
- 🔴 Crashes with >10 plugin instances
- 🔴 30,000 paint allocations/second
- 🔴 150 lines of layout code
- 🔴 Eye strain after 1 hour

**After Refactoring**:
- ✅ Stable with 50+ instances
- ✅ ~30 paint allocations/resize
- ✅ 20 lines of layout code
- ✅ Comfortable for 8+ hours

---

## Next Steps

You are currently at:
- **Week 1, Task 1.1**:- [x] Implement Quantize and Humanize tools in Piano RollySynthUI.cpp`

To continue:
1. Open `apps/desktop/Source/ui/skia/ZenithPolySynthUI.cpp`
2. Implement methods from `PHASE1_RENDER_TREE_IMPLEMENTATION.md`
3. Build and test
4. Report results

Let's ship this! 🚀
