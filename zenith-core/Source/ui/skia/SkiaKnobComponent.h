/**
 * @file SkiaKnobComponent.h
 * @brief Professional rotary knob with spring physics
 *
 * Features:
 * - Smooth rotation with spring physics
 * - Arc indicator for value
 * - Detents/notches for specific values
 * - Double-click to reset
 * - Shift for fine control
 * - Text value display
 * - 3D-style depth rendering
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include <functional>

namespace zenith {

/**
 * @class SkiaKnobComponent
 * @brief GPU-rendered rotary knob with spring physics
 */
class SkiaKnobComponent : public juce::Component,
                          private juce::Timer
{
public:
    /**
     * Knob style
     */
    enum class Style
    {
        Continuous,  ///< Full rotation (-150° to +150°)
        Stepped,     ///< Discrete steps with detents
        Bipolar      ///< Center-zero (-1 to +1)
    };

    /**
     * @brief Construct knob
     * @param style Visual/interaction style
     */
    explicit SkiaKnobComponent(Style style = Style::Continuous);

    /// Destructor
    ~SkiaKnobComponent() override;

    //==========================================================================
    // Component Interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Mouse Events
    //==========================================================================

    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event,
                       const juce::MouseWheelDetails& wheel) override;

    //==========================================================================
    // Value Control
    //==========================================================================

    /**
     * @brief Set knob value (0.0 to 1.0)
     */
    void setValue(double value, bool sendNotification = true);

    /**
     * @brief Get current value
     */
    double getValue() const { return value_; }

    /**
     * @brief Set value range
     */
    void setRange(double minimum, double maximum) [[maybe_unused]];

    /**
     * @brief Set default value (for double-click reset)
     */
    void setDefaultValue(double value) [[maybe_unused]] { defaultValue_ = value; }

    /**
     * @brief Set number of steps (for Stepped style)
     */
    void setNumSteps(int steps) [[maybe_unused]];

    /**
     * @brief Value change callback
     */
    std::function<void(double)> onValueChange;

    //==========================================================================
    // Display Options
    //==========================================================================

    /**
     * @brief Set label text (displayed below knob)
     */
    void setLabel(const juce::String& label) { label_ = label; }

    /**
     * @brief Set text suffix for value
     */
    void setTextSuffix(const juce::String& suffix) { textSuffix_ = suffix; }

    /**
     * @brief Show/hide value text
     */
    void setShowValue(bool show) [[maybe_unused]] { showValue_ = show; }

    /**
     * @brief Set value display precision
     */
    void setTextPrecision(int decimals) [[maybe_unused]] { textPrecision_ = decimals; }

    /**
     * @brief Set sensitivity (pixels per full rotation)
     */
    void setSensitivity(float sensitivity) [[maybe_unused]] { sensitivity_ = sensitivity; }

private:
    // Skia renderer
    std::unique_ptr<SkiaRenderer> renderer_;

    // Configuration
    Style style_;

    // Value state
    double value_ = 0.5;
    double minimum_ = 0.0;
    double maximum_ = 1.0;
    double defaultValue_ = 0.5;
    int numSteps_ = 0;

    // Interaction state
    bool isHovered_ = false;
    bool isDragging_ = false;
    juce::Point<int> dragStartPos_;
    double dragStartValue_;
    float sensitivity_ = 200.0f;  ///< Pixels for full rotation

    // Display options
    juce::String label_;
    juce::String textSuffix_;
    bool showValue_ = true;
    int textPrecision_ = 2;

    // Animation state (spring physics)
    float rotationAngle_ = 0.0f;      ///< Current rotation (-150° to +150°)
    float rotationVelocity_ = 0.0f;
    float hoverProgress_ = 0.0f;
    float hoverVelocity_ = 0.0f;

    // Methods
    void timerCallback() override;
    void startAnimationTimer();
    void updateAnimations();
    void drawKnob(class SkCanvas* canvas);

    double constrainValue(double value) const;
    float valueToAngle(double value) const;
    double angleToValue(float angle) const;

    juce::String getValueText() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaKnobComponent)
};

} // namespace zenith

