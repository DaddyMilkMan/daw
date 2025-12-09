# Zenith DAW - UI UX Refactor Plan

**Status:** Active
**Objective:** Transition from "Hybrid JUCE/Skia" to "Pure Skia" rendering for high-performance UI widgets.

## 🎨 Philosophy: "Neon Noir"
The UI should feel like a cyberpunk cockpit. Dark backgrounds, glowing accents, glassmorphism panels.
We are moving away from JUCE's `paint()` method for controls and using direct Skia calls.

## 🏗️ Architecture: Lightweight Widgets
Instead of every knob being a heavy `juce::Component` with its own mouse listeners and paint calls, we use a lightweight struct approach.

### The `SkiaWidget` Struct
```cpp
struct SkiaWidget {
    juce::Rectangle<float> bounds;
    juce::String name;
    float value;
    // ... binding to parameter ...
    virtual void draw(SkCanvas* canvas) = 0;
    virtual void onMouseDrag(...) = 0; 
};
```

### The `ZenithPolySynthUI` Container
This is the single `juce::Component` that handles the window. It:
1.  Maintains a list of `SkiaWidget` pointers.
2.  Overrides `mouseDown`, `mouseDrag`, `mouseUp` to route events to the active widget.
3.  Overrides `drawSkiaContent` to clear the screen and call `widget->draw(canvas)`.

## 🚀 Implementation Steps (Level 4 Refactor)

1.  **Define Widgets:** Create `ZenithUIComponents.h` with `SkiaKnob`, `SkiaSlider`, `SkiaButton`.
2.  **Update Container:** Refactor `ZenithPolySynthUI` to hold `std::vector<std::unique_ptr<SkiaWidget>>`.
3.  **Remove Legacy:** Delete the old `ZenithKnob` class (JUCE Component version).
4.  **Bind Parameters:** Ensure `SkiaWidget` can bind to `juce::RangedAudioParameter` for two-way updates.
5.  **Layout:** Implement a simple FlexBox-like or grid layout system for the widgets (Manual coordinates `x += 80` are brittle).

## ✅ Done So Far
*   Defined `SkiaWidget` base struct.
*   Implemented `SkiaKnob` drawing logic (Skia arcs, gradients, blur).
*   Refactored `ZenithPolySynthUI` to use the widget vector.
*   Implemented manual mouse routing.

## 🚧 Next Steps
*   Refine the "Glow" effect (optimize `SkMaskFilter`).
*   Implement a proper layout engine (FlexBox).
*   Add tooltips back (hover support is basic currently).
