---
description: Split mega-file ZenithUIComponents.h and polish widgets to premium quality
---

# UI Fix #4: Widget System Refactor & Polish

## MISSION
`ZenithUIComponents.h` is a **1,187-line abomination** containing knobs, sliders, buttons, modulation matrix, and tooltips all in one file. This violates every principle of maintainable code. Split it, polish each widget, eliminate duplicates.

## PRE-TASK RESEARCH (MANDATORY)

1. **Web Search**: "JUCE AudioProcessorParameter Listener thread safety"
   - Verify the atomic pattern in ZenithControl is correct
   
2. **Web Search**: "Skia knob rendering arc gradient example"
   - Find inspiration for professional knob rendering
   
3. **Web Search**: "modern synth plugin UI design Serum Vital"
   - Study Serum/Vital knob aesthetics (gradient arcs, glow, center cap)
   
4. **Web Search**: "native waveform view slider visualizer"
   - See how pro plugins show values interactively
   
5. **Web Search**: "Skia animation spring interpolation"
   - Verify spring physics implementation is correct

## CRITICAL PROBLEMS TO FIX

### Problem 1: Mega-File
One massive header = slow compilation, impossible to navigate, merge conflicts guaranteed.

### Problem 2: Duplicate Classes
- `SkiaButton` in `SkiaButton.cpp/h` (full implementation)
- `ZenithButton` in `ZenithUIComponents.h` (simpler implementation)
- PICK ONE AND DELETE THE OTHER

### Problem 3: Bare-Bones Rendering
Current knob:
```cpp
canvas->drawArc(arcRect, 135.0f, sweepAngle, false, paint);
```
That's one arc. Where's:
- Tick marks for snap values?
- Value tooltip on hover?
- Drag handle indicator?
- Bipolar mode (center-out for pan)?

### Problem 4: No Value Display
Knobs and sliders don't show their current value prominently.

## IMPLEMENTATION STEPS

### Step 1: Create Component Directory Structure
```
Source/ui/skia/components/
├── ZenithKnob.h
├── ZenithKnob.cpp
├── ZenithSlider.h
├── ZenithSlider.cpp
├── ZenithButton.h          # KEEP THE BETTER ONE
├── ZenithButton.cpp
├── ZenithDropdown.h
├── ZenithDropdown.cpp
├── ZenithToggle.h
├── ZenithToggle.cpp
├── ZenithTextInput.h
├── ZenithTextInput.cpp
├── ZenithModMatrix.h
├── ZenithModMatrix.cpp
├── ZenithTooltipOverlay.h
├── ZenithTooltipOverlay.cpp
└── ZenithControl.h          # Base class only
```

### Step 2: Extract Base Class
`ZenithControl.h` - ONLY the base class:
```cpp
#pragma once
#include "SkiaComponent.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

namespace zenith {

class ZenithControl : public SkiaComponent,
                      public juce::AudioProcessorParameter::Listener {
public:
    explicit ZenithControl(const juce::String& name);
    ~ZenithControl() override;
    
    void setParameter(juce::RangedAudioParameter* param);
    void setValue(float value, bool sendNotification = true);
    float getValue() const;
    
    void setAccentColor(SkColor color) { accentColor_ = color; }
    void setTooltip(const juce::String& text) { tooltipText_ = text; }
    juce::String getTooltip() const { return tooltipText_; }
    
    std::function<void(ZenithControl*)> onHoverStateChanged;
    
    // Listener
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

protected:
    juce::String name_;
    juce::RangedAudioParameter* parameter_ = nullptr;
    juce::NormalisableRange<float> range_{0.0f, 1.0f};
    std::atomic<float> cachedValue_{0.0f};
    SkColor accentColor_ = 0xFF00FFFF;
    juce::String tooltipText_;
    juce::String textSuffix_;
    
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
};

} // namespace zenith
```

### Step 3: Polish ZenithKnob
`ZenithKnob.h` + `ZenithKnob.cpp`:

New features to add:
1. **Value Display** - Show actual value on hover/drag
2. **Tick Marks** - Optional snap positions
3. **Bipolar Mode** - For pan, filter cutoff modulation
4. **Double-Click Reset** - To default value
5. **Fine Adjustment** - Shift+drag for precision
6. **Scroll Wheel Support**
7. **Better Glow** - Pulse animation on change

Enhanced rendering:
```cpp
void ZenithKnob::drawSkia(SkCanvas* canvas) {
    // 1. Background ring (dark groove)
    drawTrack(canvas);
    
    // 2. Tick marks (if enabled)
    if (showTicks_) drawTickMarks(canvas);
    
    // 3. Value arc with gradient
    drawValueArc(canvas);
    
    // 4. Center cap (metallic)
    drawCenterCap(canvas);
    
    // 5. Indicator line
    drawIndicator(canvas);
    
    // 6. Value display (on hover/drag)
    if (isHovered_ || isDragging_) {
        drawValueTooltip(canvas);
    }
    
    // 7. Label
    drawLabel(canvas);
}
```

### Step 4: Delete Duplicate Button
1. Compare `SkiaButton` and `ZenithButton`
2. `SkiaButton` has more features (icon support, audio reactive, size variants)
3. DELETE `ZenithButton` from `ZenithUIComponents.h`
4. Rename `SkiaButton` to `ZenithButton` for consistency
5. Update all imports

### Step 5: Add Missing Widget Types

**ZenithDropdown** - For waveform selection:
- Glass background
- Chevron indicator
- Popup menu styling

**ZenithToggle** - For on/off switches:
- Pill-shaped track
- Animated slide
- Glow on active

**ZenithTextInput** - For numeric entry:
- Click to edit
- Validate input range
- Up/down arrow increment

### Step 6: Update Imports
Create `ZenithUIComponents.h` as umbrella header:
```cpp
#pragma once
// Umbrella include for all Zenith UI components
#include "components/ZenithControl.h"
#include "components/ZenithKnob.h"
#include "components/ZenithSlider.h"
#include "components/ZenithButton.h"
#include "components/ZenithDropdown.h"
#include "components/ZenithToggle.h"
#include "components/ZenithTextInput.h"
#include "components/ZenithModMatrix.h"
#include "components/ZenithTooltipOverlay.h"
```

### Step 7: Update CMakeLists.txt
Add new source files:
```cmake
target_sources(ZenithDAW PRIVATE
    Source/ui/skia/components/ZenithControl.cpp
    Source/ui/skia/components/ZenithKnob.cpp
    Source/ui/skia/components/ZenithSlider.cpp
    Source/ui/skia/components/ZenithButton.cpp
    Source/ui/skia/components/ZenithDropdown.cpp
    Source/ui/skia/components/ZenithToggle.cpp
    Source/ui/skia/components/ZenithTextInput.cpp
    Source/ui/skia/components/ZenithModMatrix.cpp
    Source/ui/skia/components/ZenithTooltipOverlay.cpp
)
```

## VERIFICATION CHECKLIST
- [ ] ZenithUIComponents.h is now <50 lines (umbrella only)
- [ ] Each component in separate .h/.cpp pair
- [ ] No duplicate button implementations
- [ ] Knobs show value on hover
- [ ] Double-click resets to default
- [ ] Shift+drag enables fine control
- [ ] Scroll wheel changes values
- [ ] Build succeeds with no errors
- [ ] Incremental compile of one widget <5 seconds

## ACCEPTANCE CRITERIA
1. Open a synth UI with 8+ knobs
2. Hover over knob - should show current value
3. Double-click - should reset to center/default
4. Shift+drag - should move value very slowly
5. Mouse wheel over knob - should increment/decrement

All must work or task is incomplete.
