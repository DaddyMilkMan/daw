/**
 * @file ZenithLookAndFeel.h
 * @brief Custom LookAndFeel for Zenith DAW - Modern dark theme
 *
 * Replicates the visual style from the React UI:
 * - Dark, modern DAW aesthetic
 * - Subtle rounded corners
 * - Clean separators and grids
 * - Smooth hover/press states
 * - GPU-accelerated rendering via JUCE 8
 */

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * @struct ZenithColours
 * @brief Centralized color palette matching the React UI theme
 */
struct ZenithColours
{
    // Background colors (dark theme)
    static const juce::Colour backgroundDark;      // #1a1a1a - Main background
    static const juce::Colour backgroundMedium;    // #2a2a2a - Panels
    static const juce::Colour backgroundLight;     // #3a3a3a - Controls

    // Accent colors
    static const juce::Colour accent;              // #4a9eff - Primary accent
    static const juce::Colour accentHover;         // #5aafff - Hover state
    static const juce::Colour accentPressed;       // #3a8eef - Pressed state

    // Transport button colors
    static const juce::Colour playButton;          // #4ade80 - Green
    static const juce::Colour stopButton;          // #ef4444 - Red
    static const juce::Colour recordButton;        // #f87171 - Light red

    // Track/clip colors (for arrangement view)
    static const juce::Colour track1;              // #8b5cf6 - Purple
    static const juce::Colour track2;              // #ec4899 - Pink
    static const juce::Colour track3;              // #06b6d4 - Cyan
    static const juce::Colour track4;              // #f59e0b - Orange

    // Text colors
    static const juce::Colour textPrimary;         // #ffffff - Main text
    static const juce::Colour textSecondary;       // #a0a0a0 - Secondary text

    // UI element colors
    static const juce::Colour border;              // #404040 - Borders/separators
    static const juce::Colour highlight;           // #4a9eff - Selection highlight

    // Waveform colors
    static const juce::Colour waveformFill;        // #4a9eff
    static const juce::Colour waveformOutline;     // #6ab0ff

    // VU Meter colors
    static const juce::Colour meterGreen;          // #22c55e - Normal levels
    static const juce::Colour meterYellow;         // #eab308 - Warning levels
    static const juce::Colour meterRed;            // #ef4444 - Peak/clip levels
};

//==============================================================================
/**
 * @class ZenithLookAndFeel
 * @brief Custom JUCE LookAndFeel implementing Zenith DAW visual design
 *
 * Provides custom drawing for all JUCE components to match the React UI:
 * - Buttons with rounded corners and smooth gradients
 * - Rotary and linear sliders with modern styling
 * - Custom labels with proper typography
 * - Minimal scrollbars
 * - Toggle buttons for mute/solo
 * - VU meters with color gradients
 */
class ZenithLookAndFeel : public juce::LookAndFeel_V4
{
public:
    //==========================================================================
    ZenithLookAndFeel();
    ~ZenithLookAndFeel() override = default;

    //==========================================================================
    // Button drawing (transport, tools, mute/solo)
    //==========================================================================

    void drawButtonBackground(juce::Graphics& g,
                            juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g,
                       juce::TextButton& button,
                       bool shouldDrawButtonAsHighlighted,
                       bool shouldDrawButtonAsDown) override;

    //==========================================================================
    // Slider drawing (faders, knobs, pan controls)
    //==========================================================================

    void drawRotarySlider(juce::Graphics& g,
                         int x, int y,
                         int width, int height,
                         float sliderPosProportional,
                         float rotaryStartAngle,
                         float rotaryEndAngle,
                         juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g,
                         int x, int y,
                         int width, int height,
                         float sliderPos,
                         float minSliderPos,
                         float maxSliderPos,
                         const juce::Slider::SliderStyle style,
                         juce::Slider& slider) override;

    //==========================================================================
    // Label drawing (track names, value displays)
    //==========================================================================

    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    //==========================================================================
    // ComboBox drawing (dropdowns for settings)
    //==========================================================================

    void drawComboBox(juce::Graphics& g,
                     int width, int height,
                     bool isButtonDown,
                     int buttonX, int buttonY,
                     int buttonW, int buttonH,
                     juce::ComboBox& box) override;

    //==========================================================================
    // Scrollbar drawing (minimal, auto-hide style)
    //==========================================================================

    void drawScrollbar(juce::Graphics& g,
                      juce::ScrollBar& scrollbar,
                      int x, int y,
                      int width, int height,
                      bool isScrollbarVertical,
                      int thumbStartPosition,
                      int thumbSize,
                      bool isMouseOver,
                      bool isMouseDown) override;

    //==========================================================================
    // Toggle button drawing (mute/solo, loop, metro)
    //==========================================================================

    void drawToggleButton(juce::Graphics& g,
                         juce::ToggleButton& button,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    //==========================================================================
    // Custom drawing helpers
    //==========================================================================

    /**
     * @brief Draw a VU meter bar with gradient coloring
     * @param g Graphics context
     * @param bounds Rectangle to draw in
     * @param level Current level (0.0 to 1.0)
     * @param isHorizontal Orientation
     */
    static void drawLevelMeter(juce::Graphics& g,
                              juce::Rectangle<float> bounds,
                              float level,
                              bool isHorizontal = false);

    /**
     * @brief Draw a waveform display
     * @param g Graphics context
     * @param bounds Rectangle to draw in
     * @param audioData Audio sample data
     * @param numSamples Number of samples
     */
    static void drawWaveform(juce::Graphics& g,
                            juce::Rectangle<float> bounds,
                            const float* audioData,
                            int numSamples);

private:
    //==========================================================================
    // Helper methods
    //==========================================================================

    juce::Colour getButtonBackgroundColour(juce::Button& button,
                                          bool isHighlighted,
                                          bool isDown);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithLookAndFeel)
};
