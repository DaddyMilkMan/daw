---
description: How to implement interactive UI controls in Zenith DAW (ZenithControl, Skia, Parameters)
---

# Zenith DAW UI Implementation Guide

**Problem**: AI agents often fail to make UI interactive because they use standard JUCE `SliderAttachment` or raw `paint()` methods, ignoring the custom `ZenithControl` framework.

**Solution**: You MUST use `ZenithControl` (or a subclass) and the `addWidget` pattern.

## 1. The Rule of ZenithControl
ALL interactive audio parameters MUST inherit from `zenith::ZenithControl`.
- **Do NOT** use `juce::Slider` or `juce::TextButton` directly for audio parameters.
- **Do NOT** use `AudioProcessorValueTreeState::SliderAttachment`.
- `ZenithControl` handles parameter listeners, thread safety, and Skia rendering triggers automatically.

## 2. How to Add a Control

### Step A: Define the Parameter ID
Ensure the parameter ID exists in your Processor (e.g., `ZenithPolySynthProcessor::FilterCutoff`).

### Step B: Use the `addWidget` Helper
In your Editor's `buildUI()` method, use the `addWidget<Type>` helper:

```cpp
// Correct way:
addWidget<ZenithKnob>("Cutoff", ZenithPolySynthProcessor::FilterCutoff);
addWidget<ZenithSlider>("Attack", ZenithPolySynthProcessor::AmpAttack);
```

### Step C: Verify `ZenithControl` Implementation
If creating a *new* custom widget, it must:
1. Inherit `public ZenithControl`.
2. Implement `drawSkia(SkCanvas* canvas)`.
3. In `drawSkia`, use `getValue()` or `getNormalizedValue()` to drive the visual state.
4. `ZenithControl` automatically handles `mouseDown`/`drag` to update the attached parameter.

```cpp
class MyCustomKnob : public ZenithControl {
public:
    MyCustomKnob(const juce::String& name) : ZenithControl(name) {}

    void drawSkia(SkCanvas* canvas) override {
        float val = getNormalizedValue();
        // ... draw arc based on val ...
    }
};
```

## 3. The Rendering Pipeline
- **Draw Code**: Goes in `drawSkia(SkCanvas*)`, NOT `paint(juce::Graphics&)`.
- **State Access**: Use `getValue()`. Do NOT read the processor directly from the draw thread.
- **Updates**: `repaint()` in ZenithControl triggers the Skia view to redraw.

## Checklist for Agents
- [ ] Did I inherit from `ZenithControl`?
- [ ] Did I use `addWidget` or manually call `setParameter(param)`?
- [ ] Am I drawing in `drawSkia` instead of `paint`?
- [ ] Am I using `getValue()` for the visual state?
