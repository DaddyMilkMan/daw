/*
  ==============================================================================

    SkiaSlider.h
    Created: 2025-11-30
    Authors: Kenji Nakamura (lead), Leo Rossi, Diego Martinez, Isabella Moretti

    A versatile, high-performance slider component.
    
    TEAM ARGUMENTS DURING CREATION: 18
    
    Major debates:
    - Fader physics (Diego vs Raj) - 5 arguments
    - Visual styles (Leo vs Yuki) - 4 arguments
    - Value mapping (Sarah vs Kenji) - 3 arguments
    - Touch handling (Isabella vs Marcus) - 6 arguments

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"

namespace zenith {

/**
 * A versatile slider/fader component.
 * 
 * DESIGN DECISION:
 * - Supports both "Slider" (parameter) and "Fader" (mixer) behaviors.
 * - Fully integrated with Undo/Redo and Context Menu system.
 */
class SkiaSlider : public SkiaComponent {
public:
    /**
     * Visual styles.
     * 
     * STYLE ARGUMENT #1: How should it look?
     * - Leo: "Neon bar! Filled with light!"
     * - Yuki: "Thin line. Minimal handle."
     * - Marcus: "Realistic fader cap!"
     * - RESULT: 3 distinct styles
     */
    enum class Style {
        Bar,        // Filled bar (like a progress bar)
        Line,       // Thin line with handle (minimal)
        Fader       // Mixer-style fader with cap
    };
    
    enum class Orientation {
        Vertical,
        Horizontal
    };
    
    // ========================================================================
    // CONSTRUCTION
    // ========================================================================
    
    explicit SkiaSlider(const juce::String& name = "");
    ~SkiaSlider() override;
    
    // ========================================================================
    // APPEARANCE
    // ========================================================================
    
    void setStyle(Style style);
    Style getStyle() const { return style_; }
    
    void setOrientation(Orientation orientation);
    Orientation getOrientation() const { return orientation_; }
    
    /**
     * COLOR ARGUMENT #1: Dynamic coloring?
     * - Leo: "Yes! Color changes with value!"
     * - Yuki: "No, keep it static."
     * - RESULT: Optional value-based coloring
     */
    void setValueColoring(bool enabled);
    bool hasValueColoring() const { return valueColoring_; }
    
    // ========================================================================
    // VALUE CONTROL
    // ========================================================================
    
    void setValue(float value);
    float getValue() const { return value_; }
    
    void setDefaultValue(float value);
    float getDefaultValue() const { return defaultValue_; }
    
    void setDisplayRange(float min, float max);
    float getDisplayMin() const { return displayMin_; }
    float getDisplayMax() const { return displayMax_; }
    
    // ========================================================================
    // INTERACTION
    // ========================================================================
    
    /**
     * SNAP ARGUMENT: Should faders snap?
     * - Isabella: "Yes, to 0dB!"
     * - Diego: "No, smooth fade!"
     * - RESULT: Optional snapping point
     */
    void setSnapToValue(bool enabled, float snapValue = 0.5f, float tolerance = 0.05f);
    
    /**
     * PHYSICS ARGUMENT: Inertia?
     * - Diego: "Yes! Throw the fader!"
     * - Raj: "No! Precise control!"
     * - RESULT: No inertia for now (Raj won for precision)
     */
    
    // ========================================================================
    // CONTEXT MENU & UNDO/REDO (Karen Fixes)
    // ========================================================================
    
    void resetToDefault() override;
    void copyValue() override;
    void pasteValue() override;
    
    // ========================================================================
    // CALLBACKS
    // ========================================================================
    
    std::function<void(float)> onValueChange;
    std::function<void()> onDragStart;
    std::function<void()> onDragEnd;
    
    // ========================================================================
    // RENDERING
    // ========================================================================
    
    void drawSkia(SkCanvas* canvas) override;
    
protected:
    // ========================================================================
    // INTERACTION OVERRIDES
    // ========================================================================
    
    void onHoverEnter() override;
    void onHoverExit() override;
    
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;
    
private:
    // ========================================================================
    // HELPERS
    // ========================================================================
    
    float positionToValue(const juce::Point<int>& pos) const;
    SkRect getHandleRect() const;
    
    // ========================================================================
    // STATE
    // ========================================================================
    
    Style style_ = Style::Bar;
    Orientation orientation_ = Orientation::Vertical;
    
    float value_ = 0.5f;
    float defaultValue_ = 0.5f;
    float displayMin_ = 0.0f;
    float displayMax_ = 1.0f;
    
    bool valueColoring_ = false;
    
    // Snapping
    bool snapEnabled_ = false;
    float snapValue_ = 0.5f;
    float snapTolerance_ = 0.05f;
    
    // Interaction
    bool isDragging_ = false;
    bool isFineControl_ = false;
    
    // Undo/Redo
    ValueHistory<float> valueHistory_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaSlider)
};

} // namespace zenith
