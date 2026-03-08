# Phase 1 Implementation: Thread-Safe Render Tree

## Status: Week 1 - In Progress

### ✅ Completed
1. **RenderTree.h**: Core render state structures and triple-buffer frame swapping
2. **SkiaKnob.h/cpp**: Added `captureRenderState()` method for state snapshotting  
3. **ZenithPolySynthUI.h**: Added frame buffer and render methods

### 🚧 Next Steps

#### Step 1: Implement Frame Capture Logic (`ZenithPolySynthUI.cpp`)

Add these methods to `ZenithPolySynthUI.cpp`:

```cpp
// Constructor - Start timer
ZenithPolySynthUI::ZenithPolySynthUI(ZenithPolySynthProcessor &p)
    : AudioProcessorEditor(&p), 
      SkiaRenderer(this), 
      processor(p) {
  
  // ... existing initialization code ...
  
  // Start frame capture timer (60 FPS)
  startTimer(16); // ~60Hz
}

// Timer Callback - Capture UI State (MESSAGE THREAD)
void ZenithPolySynthUI::timerCallback() {
    captureFrameSnapshot();
}

void ZenithPolySynthUI::captureFrameSnapshot() {
    // Get writable buffer (Message Thread safe)
    auto* frame = frameBuffer_.getWriteBuffer();
    frame->clear();
    
    // Capture global state
    frame->isAdvancedMode = isAdvancedMode_;
    frame->componentBounds = getLocalBounds();
    frame->frameNumber++;
    frame->timestamp = juce::Time::getCurrentTime().toMilliseconds();
    
    // Snapshot all knobs (safe - we're on Message Thread)
    if (cutoffKnob_) {
        frame->knobs.push_back(cutoffKnob_->captureRenderState());
    }
    if (resKnob_) {
        frame->knobs.push_back(resKnob_->captureRenderState());
    }
    if (envAmtKnob_) {
        frame->knobs.push_back(envAmtKnob_->captureRenderState());
    }
    if (subLevelKnob_) {
        frame->knobs.push_back(subLevelKnob_->captureRenderState());
    }
    if (noiseLevelKnob_) {
        frame->knobs.push_back(noiseLevelKnob_->captureRenderState());
    }
    
    // TODO: Snapshot sliders, buttons, visualizer, etc.
    
    // Swap to ready (atomic, lock-free)
    frameBuffer_.swapWriteToReady();
}

// Render Thread - Draw from Snapshot (OPENGL THREAD)
void ZenithPolySynthUI::drawSkiaContent(SkCanvas* canvas) {
    if (!canvas) return;
    
    // Get latest frame (lock-free read)
    const auto* frame = frameBuffer_.getLatestFrame();
    
    // Clear background
    canvas->clear(SkColorSetRGB(20, 20, 25));
    
    // Draw from snapshot (NO Component access!)
    for (const auto& knob : frame->knobs) {
        drawKnobFromState(canvas, knob);
    }
    
    // TODO: Draw sliders, buttons, visualizer from state
}

void ZenithPolySynthUI::drawKnobFromState(SkCanvas* canvas, const render::KnobRenderState& state) {
    if (state.bounds.isEmpty()) return;
    
    canvas->save();
    
    // Apply hover scale
    if (state.scale > 1.0f) {
        float cx = state.bounds.centerX();
        float cy = state.bounds.centerY();
        canvas->translate(cx, cy);
        canvas->scale(state.scale, state.scale);
        canvas->translate(-cx, -cy);
    }
    
    // Calculate knob geometry
    float cx = state.bounds.centerX();
    float cy = state.bounds.centerY();
    float radius = std::min(state.bounds.width(), state.bounds.height()) * 0.35f;
    SkRect arcRect = SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    
    // Arc parameters
    float startAngle = -135.0f; // -270/2 - 90
    float sweepAngle = state.value * 270.0f;
    
    // Background track
    SkPaint trackPaint;
    trackPaint.setStyle(SkPaint::kStroke_Style);
    trackPaint.setStrokeWidth(2.5f);
    trackPaint.setColor(SkColorSetARGB(77, 255, 255, 255)); // 30% white
    trackPaint.setAntiAlias(true);
    trackPaint.setStrokeCap(SkPaint::kRound_Cap);
    canvas->drawArc(arcRect, startAngle, 270.0f, false, trackPaint);
    
    // Glow layer (if hovered)
    if (state.glowIntensity > 0.01f) {
        SkPaint glowPaint;
        glowPaint.setStyle(SkPaint::kStroke_Style);
        glowPaint.setStrokeWidth(5.0f);
        glowPaint.setColor(SkColorSetA(state.glowColor, static_cast<U8CPU>(state.glowIntensity * 102))); // 40% max alpha
        glowPaint.setAntiAlias(true);
        glowPaint.setStrokeCap(SkPaint::kRound_Cap);
        canvas->drawArc(arcRect, startAngle, sweepAngle, false, glowPaint);
    }
    
    // Value arc
    SkPaint valuePaint;
    valuePaint.setStyle(SkPaint::kStroke_Style);
    valuePaint.setStrokeWidth(2.5f);
    valuePaint.setColor(state.baseColor);
    valuePaint.setAntiAlias(true);
    valuePaint.setStrokeCap(SkPaint::kRound_Cap);
    canvas->drawArc(arcRect, startAngle, sweepAngle, false, valuePaint);
    
    // Label (if provided)
    if (state.labelText.isNotEmpty()) {
        SkFont font;
        font.setSize(12.0f);
        
        SkPaint textPaint;
        textPaint.setColor(SkColorSetRGB(160, 160, 165));
        
        float textY = cy + radius + 15.0f;
        float width = font.measureText(state.labelText.toRawUTF8(), state.labelText.length(), SkTextEncoding::kUTF8);
        canvas->drawString(state.labelText.toRawUTF8(), cx - width / 2.0f, textY, font, textPaint);
    }
    
    canvas->restore();
}
```

#### Step 2: Verify Thread Safety

Add debugging to confirm threads are separated:

```cpp
void ZenithPolySynthUI::captureFrameSnapshot() {
    // Debug: Verify we're on Message Thread
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    // ... rest of implementation
}

void ZenithPolySynthUI::drawSkiaContent(SkCanvas* canvas) {
    // Debug: Verify we're NOT on Message Thread
    jassert(!juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    // ... rest of implementation
}
```

#### Step 3: Test & Profile

1. **Crash Test**: Open/close menus, tooltips, change presets rapidly while rendering
2. **Performance Test**: Measure frame times with 100+ knobs
3. **Thread Sanitizer**: Run with `-fsanitize=thread` (if available)

### 📊 Expected Results

**Before**:
- ❌ Race conditions when iterating `getChildren()`
- ❌ Random crashes during UI changes
- ❌ Cannot render on separate thread safely

**After**:
- ✅ Zero Component access from Render Thread
- ✅ Lock-free frame swapping (triple buffer)
- ✅ Stable with any UI changes during rendering
- ✅ Foundation for 144Hz rendering

### 🔄 Integration with Existing Code

The current `ZenithPolySynthUI.cpp` uses a "widget" pattern with `addWidget<>()`. We have two options:

**Option A: Hybrid Approach** (Recommended for incremental migration)
- Keep existing widgets for now
- Add `captureRenderState()` methods to each widget type
- Gradually migrate rendering to use snapshots

**Option B: Full Rewrite** (Clean slate, more work)
- Remove widget abstraction
- Use direct Component ownership (like old code)
- Implement full render tree from scratch

**I recommend Option A** for safer, incremental refactoring.

### 📝 Next Implementation Session

When ready, I'll:
1. Implement the methods above in `ZenithPolySynthUI.cpp`
2. Add `captureRenderState()` to `SkiaSlider`, `ZenithButton`, etc.
3. Update `drawSkiaContent()` to use full frame snapshot
4. Add performance profiling code
5. Test with ThreadSanitizer

---

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    Message Thread (JUCE)                     │
├─────────────────────────────────────────────────────────────┤
│  Timer (60Hz):                                               │
│    1. captureFrameSnapshot()                                 │
│       → Read Component states (knobs, sliders, buttons)      │
│       → Build UiFrameData                                    │
│    2. frameBuffer_.swapWriteToReady()                        │
│       → Atomic swap to ready buffer                          │
└─────────────────────────────────────────────────────────────┘
                            ↓ (lock-free)
┌─────────────────────────────────────────────────────────────┐
│              Triple Buffer (Atomic Swaps)                    │
├─────────────────────────────────────────────────────────────┤
│  [Write Buffer] ← Message Thread writes here                 │
│  [Ready Buffer] ← Completed frames                           │
│  [Render Buffer] ← Render Thread reads here                  │
└─────────────────────────────────────────────────────────────┘
                            ↓ (lock-free)
┌─────────────────────────────────────────────────────────────┐
│                OpenGL Thread (Skia Rendering)                │
├─────────────────────────────────────────────────────────────┤
│  Render Loop:                                                │
│    1. frame = frameBuffer_.getLatestFrame()                  │
│       → Read latest snapshot (NO Component access)           │
│    2. drawSkiaContent(canvas)                                │
│       → Draw knobs from state                                │
│       → Draw sliders from state                              │
│       → Draw buttons from state                              │
│    3. present()                                              │
└─────────────────────────────────────────────────────────────┘
```

**Key Properties**:
- ✅ Zero locks/mutexes
- ✅ Zero Component access from Render Thread
- ✅ Lock-free atomic swaps
- ✅ Latest frame always available
- ✅ Zero frame drops (triple buffer vs double buffer)

---

## Testing Plan

### Test 1: Crash Resistance
```cpp
// Rapidly add/remove components while rendering
for (int i = 0; i < 1000; i++) {
    // Message Thread
    if (i % 2 == 0) {
        addAndMakeVisible(newKnob);
    } else {
        removeChildComponent(knob);
    }
    juce::Thread::sleep(1); // Let render thread catch up
}
// Expected: ZERO crashes
```

### Test 2: Performance
```cpp
// Measure frame capture time
auto start = juce::Time::getHighResolutionTicks();
captureFrameSnapshot();
auto end = juce::Time::getHighResolutionTicks();
auto microseconds = juce::Time::highResolutionTicksToSeconds(end - start) * 1'000'000;

DBG("Frame capture: " + juce::String(microseconds) + " μs");
// Target: < 500 μs for 100 controls
```

### Test 3: Thread Sanitizer
```bash
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
./ZenithDAW
# Expected: ZERO thread safety warnings
```

Complete this phase, then move to Phase 2 (Performance Caching).
