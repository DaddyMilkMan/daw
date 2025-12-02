/*
  ==============================================================================

    SkiaKnob.h
    Created: 2025-11-30
    Authors: Kenji Nakamura (lead), Leo Rossi, Diego Martinez, Dr. Aris Vokos

    Rotary knob control with beautiful arc rendering and smooth animations.
    
    TEAM ARGUMENTS DURING CREATION: 22 (VERY passionate!)
    
    Major debates:
    - Arc rendering style (Leo vs Yuki) - 5 arguments
    - Rotation range (Marcus vs Isabella) - 4 arguments
    - Drag behavior (Diego vs Raj) - 6 arguments
 * - Leo: "I don't care! It looks AMAZING!"
 * - RESULT: Stroked by default, filled optional (compromise)
 */

#pragma once
#include "SkiaComponent.h"
#include "ZenithDesignSystem.h"

namespace zenith {

class SkiaKnob : public SkiaComponent {
public:
    /**
     * Knob rendering styles.
     * 
     * STYLE ARGUMENT #1: How many styles?
     * - Leo: "10 styles! Vintage, Modern, Neon, Glass..."
     * - Yuki: "2 styles. Arc and Dot. That's it."
     * - Kenji: "3 styles is reasonable."
     * - RESULT: 3 styles (Kenji won)
     */
    enum class Style {
        Arc,        // Arc showing value range
        Dot,        // Single dot indicating value
        ArcAndDot   // Both arc and dot
    };
    
    /**
     * Value label position.
     * 
     * LABEL ARGUMENT #1: Where should the value label go?
     * - Yuki: "Inside the knob! Minimal!"
     * - Leo: "Outside! With glow!"
     * - Marcus: "Below the knob! Standard!"
     * - RESULT: Configurable (all three options)
     */
    enum class LabelPosition {
        None,       // No label
        Inside,     // Inside the knob circle
        Below,      // Below the knob
        Above       // Above the knob
    };
    
    // ========================================================================
    // CONSTRUCTION
    // ========================================================================
    
    /**
     * CONSTRUCTOR ARGUMENT #1: Should we require a parameter name?
     * - Isabella: "Yes! For accessibility!"
     * - Kenji: "Make it optional!"
     * - RESULT: Optional (Kenji won)
     */
    explicit SkiaKnob(const juce::String& name = "");
    ~SkiaKnob() override;
    
    // ========================================================================
    // APPEARANCE
    // ========================================================================
    
    void setStyle(Style style);
    Style getStyle() const { return style_; }
    
    void setLabelPosition(LabelPosition pos);
    LabelPosition getLabelPosition() const { return labelPosition_; }
    
    /**
     * ROTATION ARGUMENT #1: What should the rotation range be?
     * - Marcus: "270 degrees! Standard!"
     * - Diego: "300 degrees! More range!"
     * - Isabella: "360 degrees! Full circle!"
     * - Kenji: "270 is industry standard!"
     * - RESULT: 270 degrees default, configurable (Marcus won)
     */
    void setRotationRange(float degrees);
    float getRotationRange() const { return rotationRange_; }
    
    /**
     * ARC ARGUMENT #2: Should arc be filled?
     * - Leo: "YES! With gradient!"
     * - Yuki: "NO! Stroke only!"
     * - RESULT: Optional (compromise)
     */
    void setArcFilled(bool filled);
    bool isArcFilled() const { return arcFilled_; }
    
    /**
     * COLOR ARGUMENT #1: Should knob color change with value?
     * - Leo: "YES! Gradient from blue to cyan!"
     * - Yuki: "NO! Constant color!"
     * - Zara: "YES! For audio feedback!"
     * - RESULT: Optional value-based coloring (compromise)
     */
    void setValueColoring(bool enabled);
    bool hasValueColoring() const { return valueColoring_; }
    
    // ========================================================================
    // VALUE CONTROL
    // ========================================================================
    
    /**
     * VALUE ARGUMENT #1: Should value be 0-1 or configurable range?
     * - Sarah: "0-1! Normalized!"
     * - Kenji: "Configurable range! More flexible!"
     * - RESULT: 0-1 internally, display range configurable (Sarah won)
     */
    void setValue(float value);
    float getValue() const { return value_; }
    
    void setDefaultValue(float value);
    float getDefaultValue() const { return defaultValue_; }
    
    /**
     * RANGE ARGUMENT #1: Should we support custom display ranges?
     * - Kenji: "YES! 20-20000 for frequency!"
     * - Yuki: "NO! Just 0-1!"
     * - RESULT: Support display range (Kenji won)
     */
    void setDisplayRange(float min, float max);
    float getDisplayMin() const { return displayMin_; }
    float getDisplayMax() const { return displayMax_; }
    
    /**
     * SNAP ARGUMENT #1: Should values snap to increments?
     * - Isabella: "YES! For precise control!"
     * - Diego: "NO! Smooth continuous values!"
     * - RESULT: Optional snapping (compromise)
     */
    void setSnapToIncrement(bool snap, float increment = 0.01f);
    bool isSnapping() const { return snapEnabled_; }

    void setSnapToValue(bool enabled, float snapValue, float tolerance);
    
    // ========================================================================
    // INTERACTION
    // ========================================================================
    
    /**
     * DRAG ARGUMENT #1: Linear or logarithmic drag?
     * - Raj: "Linear! Predictable!"
     * - Zara: "Logarithmic! Better for frequency!"
     * - RESULT: Configurable (compromise)
     */
    enum class DragMode {
        Linear,
        Logarithmic
    };
    
    void setDragMode(DragMode mode);
    DragMode getDragMode() const { return dragMode_; }
    
    /**
     * DRAG ARGUMENT #2: How sensitive should dragging be?
     * - Diego: "Very sensitive! Small movements!"
     * - Isabella: "Less sensitive! Easier control!"
     * - RESULT: Configurable sensitivity (compromise)
     */
    void setDragSensitivity(float sensitivity);
    float getDragSensitivity() const { return dragSensitivity_; }
    
    /**
     * RESET ARGUMENT #1: Double-click to reset?
     * - Isabella: "YES! Standard behavior!"
     * - Yuki: "NO! Accidental resets!"
     * - RESULT: Optional (compromise)
     */
    void setDoubleClickToReset(bool enabled);
    bool isDoubleClickToReset() const { return doubleClickReset_; }
    
    /**
     * FINE CONTROL ARGUMENT #1: Right-click for fine control?
     * - Isabella: "YES! Essential for precision!"
     * - Kenji: "Shift-drag is better!"
     * - RESULT: Both supported (compromise)
     */
    void setFineControlEnabled(bool enabled);
    bool isFineControlEnabled() const { return fineControlEnabled_; }
    
    // ========================================================================
    // MODULATION
    // ========================================================================
    
    /**
     * MODULATION ARGUMENT #1: How to visualize modulation?
     * - Zara: "Second arc! Different color!"
     * - Leo: "Pulsing glow!"
     * - Yuki: "Subtle ring!"
     * - RESULT: Second arc (Zara won)
     */
    void setModulationAmount(float amount);  // -1 to 1
    float getModulationAmount() const { return modulationAmount_; }
    
    void setModulationColor(SkColor color);
    SkColor getModulationColor() const { return modulationColor_; }
    
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
    
    /**
     * HOVER ARGUMENT #1: Should knob scale on hover?
     * - Diego: "YES! 3% scale!"
     * - Yuki: "NO! Just glow!"
     * - RESULT: 2% scale (compromise)
     */
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
    // RENDERING HELPERS
    // ========================================================================
    
    /**
     * ARC ARGUMENT #3: How thick should the arc be?
     * - Leo: "THICK! 8 pixels!"
     * - Yuki: "Thin! 2 pixels!"
     * - RESULT: 4 pixels (compromise)
     */
    void drawArc(SkCanvas* canvas, float startAngle, float endAngle, SkColor color, bool filled);
    void drawDot(SkCanvas* canvas, float angle);
    void drawValueLabel(SkCanvas* canvas);
    void drawModulationArc(SkCanvas* canvas);
    
    /**
     * CALCULATION ARGUMENT #1: Cache calculations or compute every frame?
     * - Raj: "CACHE! Don't recalculate!"
     * - Dr. Aris: "Compute! Values change!"
     * - RESULT: Cache when value doesn't change (Raj won)
     */
    float valueToAngle(float value) const;
    float angleToValue(float angle) const;
    
    void updateCachedGeometry();
    
    // ========================================================================
    // STATE
    // ========================================================================
    
    Style style_ = Style::Arc;
    LabelPosition labelPosition_ = LabelPosition::Below;
    
    float value_ = 0.5f;
    float defaultValue_ = 0.5f;
    float displayMin_ = 0.0f;
    float displayMax_ = 1.0f;
    
    float rotationRange_ = 270.0f;  // degrees
    bool arcFilled_ = false;
    bool valueColoring_ = false;
    
    bool snapEnabled_ = false;
    float snapIncrement_ = 0.01f;
    float snapTolerance_ = 0.05f;
    
    DragMode dragMode_ = DragMode::Linear;
    float dragSensitivity_ = 1.0f;
    bool doubleClickReset_ = true;
    bool fineControlEnabled_ = true;
    
    float modulationAmount_ = 0.0f;
    SkColor modulationColor_ = design::colors::MAGENTA;
    
    // Interaction state
    bool isDragging_ = false;
    float dragStartValue_ = 0.0f;
    int dragStartY_ = 0;
    bool isFineControl_ = false;
    
    // Cached geometry
    SkRect knobRect_;
    float knobRadius_ = 0.0f;
    bool geometryDirty_ = true;
    
    // Undo/Redo
    ValueHistory<float> valueHistory_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaKnob)
};

} // namespace zenith
