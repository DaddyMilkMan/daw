---
description: UI/UX Professional Refactoring Workflow
---

# UI/UX Professional Refactoring Workflow

## Overview
This workflow addresses the "Mona Lisa with a crayon" issues in the UI system by eliminating rendering schizophrenia, magic number layouts, per-frame GPU burns, and manual input handling.

## Prerequisites
- Build must be passing
- Backup current UI code before proceeding

## Steps

### 1. Create Pro-Tier Skia Component Architecture

**Goal**: Establish clean separation between JUCE logic and Skia rendering

**Tasks**:
- Review current `SkiaComponent.h` interface (already exists)
- Ensure all Skia components properly implement `drawSkia(SkCanvas* canvas)`
- Document the rendering contract

**Verification**: All Skia components inherit from SkiaComponent interface

---

### 2. Refactor Knob Components for Hybrid JUCE+Skia

**Goal**: Make knobs use JUCE's battle-tested input handling while rendering with Skia

**Current Issues**:
- `SkiaKnob` inherits from `SkiaComponent` only
- Manual mouse handling (`mouseDown`, `mouseDrag`, `mouseUp`)
- Reinventing drag physics, velocity, sensitivity

**Solution**:
```cpp
// Option A: Create new hybrid base (recommended)
class SkiaSliderComponent : public juce::Slider, public SkiaComponent {
    void paint(juce::Graphics&) override {} // Empty - Skia handles it
    // Implement drawSkia() to read getValue() and render
};

// Option B: Wrap existing (quicker but less clean)
class SkiaKnobWrapper : public juce::Slider {
   SkiaKnob* visual_;
   void paint(juce::Graphics& g) override {
       // Get Skia canvas and delegate to visual_
   }
};
```

**Files to Modify**:
- `Source/ui/skia/SkiaKnob.h` - Add juce::Slider inheritance
- `Source/ui/skia/SkiaKnob.cpp` - Remove ALL mouse handling code
- `Source/ui/skia/ZenithUIComponents.h` - Update ZenithKnob similarly

**Verification**: Mouse dragging handled by JUCE, rendering by Skia

---

### 3. Implement SkPicture Caching for Static Elements

**Goal**: Eliminate per-frame expensive blur operations

**Current Issue**:
```cpp
// EVERY FRAME for EVERY KNOB = 3000 blurs/second at 60fps x 50 knobs
glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
canvas->drawArc(..., glowPaint);
```

**Solution**:
```cpp
class CachedSkiaKnob {
    sk_sp<SkPicture> backgroundCache_; // Cached static elements
    
    void resized() override {
        SkPictureRecorder recorder;
        SkCanvas* c = recorder.beginRecording(getWidth(), getHeight());
        drawStaticBackground(c); // Draw track, ticks, glow ONCE
        backgroundCache_ = recorder.finishRecordingAsPicture();
    }
    
    void drawSkia(SkCanvas* canvas) override {
        // Zero-cost blit
        if (backgroundCache_) canvas->drawPicture(backgroundCache_);
        
        // Only draw dynamic arc (changes with value)
        drawValueIndicator(canvas, getValue ());
    }
};
```

**Files to Modify**:
- `Source/ui/skia/SkiaKnob.cpp::drawSkia` - Split into static/dynamic
- `Source/ui/skia/ZenithUIComponents.h::ZenithKnob::drawSkia` - Add caching

**Verification**: Profile and confirm blur only happens on resize, not every frame

---

### 4. Delete ZenithLookAndFeel Dead Code

**Goal**: Remove unused CPU rendering path

**Files to Modify**:
- `Source/ui/ZenithLookAndFeel.cpp`:
  - Delete or stub: `drawRotarySlider()`
  - Delete or stub: `drawLinearSlider()`
  - Keep: Color definitions, spacing constants, font getters

**Rationale**: If you're using Skia for rendering, Z enithLookAndFeel's drawing code is dead weight. Keep only the design tokens (colors, spacing, fonts).

**Verification**: Build succeeds, no regression in UI appearance

---

### 5. Convert Layout to FlexBox System

**Goal**: Eliminate all hardcoded pixel coordinates

**Current Issue** (from `ZenithPolySynthUI::resized()`):
```cpp
lfo1RateKnob_->setBounds(x, y, knobSize, knobSize);
lfo1AmountKnob_->setBounds(x + 80, y, knobSize, knobSize); //  Magic 80!
x += 180; // Magic 180!
```

**Solution**:
```cpp
void ZenithPolySynthUI::resized() {
    using namespace juce;
    
    FlexBox mainLayout;
    mainLayout.flexDirection = FlexBox::Direction::column;
    
    // Top: Preset Bar (fixed height)
    mainLayout.items.add(FlexItem(*presetBar_).withHeight(40).withMargin(10));
    
    // Middle: Visualizer (flexible)
    mainLayout.items.add(FlexItem(*visualizer_).withFlex(1.0f).withMargin(10));
    
    // Bottom: Controls (fixed height, centered)
    FlexBox controlsRow;
    controlsRow.justifyContent = FlexBox::JustifyContent::center;
    controlsRow.items.add(FlexItem(*cutoffKnob_).withWidth(120).withHeight(120).withMargin(10));
    controlsRow.items.add(FlexItem(*resKnob_).withWidth(80).withHeight(80).withMargin(10));
    // ... add all controls with semantic sizing
    
    mainLayout.items.add(FlexItem(controlsRow).withHeight(200));
    
    //  Perform layout
    mainLayout.performLayout(getLocalBounds());
}
```

**Files to Modify**:
- `Source/ui/skia/ZenithPolySynthUI.cpp::resized()` - Complete rewrite with FlexBox
- Remove ALL `setBounds(x, y, w, h)` calls with magic numbers
- Use FlexBox with semantic sizing (flex, withWidth, withMargin)

**Verification**: UI scales properly at 1080p, 1440p, 4K resolutions

---

### 6. Delete "Learn" Button and Implement Proper Tooltips

**Goal**: Remove UX crutch and add professional tooltip system

**Current Issue**:
- `learningModeButton_` toggles tooltip overlay
- Admission that UI is confusing

**Solution**:
```cpp
// Delete learningModeButton_ completely
// Add global TooltipWindow or status bar

class ZenithPolySynthUI {
    std::unique_ptr<juce::TooltipWindow> tooltipWindow_;
    std::unique_ptr<StatusBar> statusBar_;
    
    // In constructor:
    tooltipWindow_ = std::make_unique<juce::TooltipWindow>(this, 700);
    statusBar_ = std::make_unique<StatusBar>();
    
    // Controls automatically show tooltips on hover via setTooltip()
    cutoffKnob_->setTooltip("Filter Cutoff: Controls brightness");
    
    // Advanced: Show live value in status bar
    cutoffKnob_->onMouseEnter = [this]() {
        float hz = cutoffKnob_->getValue() * 20000.0f;
        statusBar_->setText(juce::String::formatted("Cutoff: %.1f Hz", hz));
    };
};
```

**Files to Modify**:
- `Source/ui/skia/ZenithPolySynthUI.h`:
  - Remove: `learningModeButton_`
  - Remove: `isLearningMode_`
  - Remove: `toggleLearningMode()`
  - Add: `std::unique_ptr<juce::TooltipWindow> tooltipWindow_`
- `Source/ui/skia/ZenithPolySynthUI.cpp`:
  - Delete all learningModeButton_ code
  - Delete ZenithTooltipOverlay usage (or repurpose for status bar)
  - Add tooltipWindow_ initialization

**Verification**: Hover over knob → instant tooltip appears without button press

---

### 7. Performance Profiling and Verification

**Goal**: Confirm we hit professional performance targets

**Metrics to Measure**:
- Frame time: Target <10ms (was ~22ms)
- GPU fill rate: Near-zero blur operations per frame
- Input latency: Target <5ms
- Memory allocations during render loop: Zero

**Tools**:
- JUCE's built-in OpenGL stats
- Visual Studio Performance Profiler
- GPU debugger (RenderDoc, NSight)

**Success Criteria**:
- [ ] 144Hz capable (6.9ms frame budget)
- [ ] Zero per-frame allocations
- [ ] Zero per-frame blur operations
- [ ] JUCE handles ALL input
- [ ] Skia handles ALL rendering
- [ ] No hardcoded pixel coordinates
- [ ] No "Learn" button

---

## Implementation Order

1. **Audit current state** - Understand what's actually used
2. **Cache Skia rendering** (Step 3) - Biggest perf win, lowest risk
3. **Delete dead code** (Step 4) - Cleanup
4. **FlexBox layout** (Step 5) - Big but mechanical change
5. **Delete Learn button** (Step 6) - UX improvement
6. **Refactor input** (Step 2) - Most complex, do last
7. **Profile** (Step 7) - Verify improvements

## Rollback Plan

If build breaks:
1. Revert CMakeLists.txt changes
2. Restore from git: `git checkout HEAD -- apps/desktop/Source/ui/`
3. Rebuild: `cmake --build build --config Release`

## Notes

- Keep ZenithLookAndFeel color/spacing definitions (design tokens)
- Delete ZenithLookAndFeel drawing code (dead weight)
- Test on multiple resolutions (1080p, 1440p, 4K)
- Test with touch input, tablet, mouse wheel
- Accessibility: Ensure keyboard navigation still works

## References

- JUCE FlexBox tutorial: https://docs.juce.com/master/tutorial_flex_box_and_grid.html
- Skia SkPicture docs: https://api.skia.org/classSkPicture.html
- Original roast doc: `docs/NUCLEAR_CODEBASE_ROAST_2025.md`
- Refactor plan: `docs/UI_UX_REFACTOR_PLAN.md`
