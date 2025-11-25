/**
 * @file SkiaSliderComponent.h
 * @brief Professional slider with spring physics and GPU rendering
 *
 * Features:
 * - Smooth spring physics for natural motion
 * - Horizontal/vertical orientation
 * - Value snapping at intervals
 * - Tick marks for stepped values
 * - Hover and drag states with glow
 * - Text value display
 * - Apple-inspired design
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
#include "../../rendering/SkiaRenderer.h"
#include <functional>

namespace zenith {

/**
 * @class SkiaSliderComponent
 * @brief GPU-rendered slider with spring physics
 *
 * A professional-quality slider that uses Skia for rendering and
 * spring physics for smooth, natural animations.
 */
class SkiaSliderComponent : public juce::Component,
                            private juce::Timer
{
public:
    /**
     * Slider orientation
     */
    enum class Orientation
    {
        Horizontal,
        Vertical
    };

    /**
     * Visual style
     */
    enum class Style
    {
        Linear,      ///< Standard linear slider
        Stepped,     ///< Discrete steps with tick marks
        Bipolar      ///< Center-zero (-1 to +1)
    };

    /**
     * @brief Construct slider
     * @param orientation Horizontal or vertical
     * @param style Visual style
     */
    explicit SkiaSliderComponent(Orientation orientation = Orientation::Horizontal,
                                Style style = Style::Linear);

    /// Destructor
    ~SkiaSliderComponent() override;

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
    void mouseWheelMove(const juce::MouseEvent& event,
                       const juce::MouseWheelDetails& wheel) override;

    //==========================================================================
    // Timer Interface
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // Value Control
    //==========================================================================

    /**
     * @brief Set slider value (0.0 to 1.0)
     */
    void setValue(double value, bool sendNotification = true);

    /**
     * @brief Get current value
     */
    double getValue() const { return value_; }

    /**
     * @brief Set value range
     */
    void setRange(double minimum, double maximum, double interval = 0.0);

    /**
     * @brief Get minimum value
     */
    double getMinimum() const { return minimum_; }

    /**
     * @brief Get maximum value
     */
    double getMaximum() const { return maximum_; }

    /**
     * @brief Set number of steps (for Stepped style)
     */
    void setNumSteps(int steps) [[maybe_unused]];

    /**
     * @brief Enable/disable value snapping
     */
    void setSnapToValue(bool snap) [[maybe_unused]] { snapToValue_ = snap; }

    /**
     * @brief Value change callback
     */
    std::function<void(double)> onValueChange;

    //==========================================================================
    // Display Options
    //==========================================================================

    /**
     * @brief Set text suffix (e.g., "dB", "Hz", "%")
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

private:
    // Skia renderer
    std::unique_ptr<SkiaRenderer> renderer_;

    // Configuration
    Orientation orientation_;
    Style style_;

    // Value state
    double value_ = 0.5;
    double minimum_ = 0.0;
    double maximum_ = 1.0;
    double interval_ = 0.0;
    int numSteps_ = 0;
    bool snapToValue_ = false;

    // Interaction state
    bool isHovered_ = false;
    bool isDragging_ = false;
    juce::Point<int> dragStartPos_;
    double dragStartValue_;

    // Display options
    juce::String textSuffix_;
    bool showValue_ = true;
    int textPrecision_ = 2;

    // Animation state (spring physics)
    float thumbProgress_ = 0.5f;      ///< Current thumb position (0-1)
    float thumbVelocity_ = 0.0f;
    float hoverProgress_ = 0.0f;
    float hoverVelocity_ = 0.0f;

    // Methods
    void startAnimationTimer();
    void updateAnimations();
    void drawSlider(class SkCanvas* canvas);

    double constrainValue(double value) const;
    double valueToPosition(double value) const;
    double positionToValue(double position) const;
    juce::Point<float> getThumbPosition() const;

    juce::String getValueText() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaSliderComponent)
};

} // namespace zenith

