/**
 * @file ZenithLookAndFeel.h
 * @brief Custom LookAndFeel for Zenith DAW
 *
 * Implements a modern, dark design system inspired by:
 * - Ableton Live (dark greys, subtle gradients, flat buttons)
 * - Bitwig Studio (teal accents, minimal borders, clean typography)
 * - Studio One (well-organized layout, consistent spacing)
 *
 * Design principles:
 * - 8px spacing grid (4/8/16/24/32px multiples)
 * - Dark, slightly desaturated color palette
 * - Teal/cyan accent for interactive elements
 * - Flat UI with subtle shadows and hover states
 * - Clean, modern typography (sans-serif)
 */

#pragma once

#include <JuceHeader.h>

namespace zenith {

/**
 * @class ZenithLookAndFeel
 * @brief Custom JUCE LookAndFeel implementing Zenith DAW's design system
 */
class ZenithLookAndFeel : public juce::LookAndFeel_V4
{
public:
    //==========================================================================
    // Constructor
    //==========================================================================
    
    ZenithLookAndFeel();
    ~ZenithLookAndFeel() override = default;

    //==========================================================================
    // Design Tokens: Colors
    //==========================================================================
    
    struct Colors
    {
        // Backgrounds - Logic Pro gray palette
        static constexpr juce::uint32 backgroundDark = 0xff1F1F1F;      // Main window background (Logic gray)
        static constexpr juce::uint32 backgroundMid = 0xff2B2B2B;       // Panels (browser, mixer)
        static constexpr juce::uint32 backgroundLight = 0xff3E3E3E;     // Hovered items, raised surfaces
        static constexpr juce::uint32 backgroundPanel = 0xff262626;     // Panel backgrounds
        
        // Accents - Logic Pro color scheme
        static constexpr juce::uint32 accentPrimary = 0xff006FFF;       // Logic Blue - primary interactive
        static constexpr juce::uint32 accentPrimaryDark = 0xff0055CC;   // Darker Logic Blue for pressed state
        static constexpr juce::uint32 accentSecondary = 0xff3A7DFF;     // Logic Blue glow - focus ring
        static constexpr juce::uint32 accentWarning = 0xffFF9900;       // Orange - warnings
        static constexpr juce::uint32 accentDanger = 0xffFF0000;        // Red - destructive/record
        
        // Text - Logic Pro softer white
        static constexpr juce::uint32 textPrimary = 0xffDFDFDF;         // Main text (softer than pure white)
        static constexpr juce::uint32 textSecondary = 0xff9A9A9A;       // Subtle text, labels
        static constexpr juce::uint32 textDisabled = 0xff666666;        // Disabled state
        
        // Borders - Logic Pro engraved style
        static constexpr juce::uint32 border = 0xff111111;              // Default borders (dark engraved)
        static constexpr juce::uint32 borderLight = 0xff444444;         // Lighter borders
        
        // Transport/Status - Logic Pro exact colors
        static constexpr juce::uint32 playGreen = 0xff00FF00;           // Play button (bright green)
        static constexpr juce::uint32 recordRed = 0xffFF0000;           // Record button (pure red)
        static constexpr juce::uint32 stopGrey = 0xff888888;            // Stop button
        
        // Meters - Logic Pro gradient
        static constexpr juce::uint32 meterGreen = 0xff00FF00;          // Level meter (low)
        static constexpr juce::uint32 meterYellow = 0xffFFFF00;         // Level meter (mid)
        static constexpr juce::uint32 meterRed = 0xffFF0000;            // Level meter (clip)
    };

    //==========================================================================
    // Design Tokens: Metrics
    //==========================================================================
    
    struct Metrics
    {
        // Spacing (8px grid)
        static constexpr int spacingXS = 4;
        static constexpr int spacingS = 8;
        static constexpr int spacingM = 16;
        static constexpr int spacingL = 24;
        static constexpr int spacingXL = 32;
        
        // Border radius - Logic Pro style (more rounded)
        static constexpr float radiusS = 2.0f;
        static constexpr float radiusM = 4.0f;      // Scrollbars
        static constexpr float radiusL = 6.0f;      // Buttons, regions (Logic standard)
        static constexpr float radiusXL = 8.0f;
        
        // Component heights - Logic Pro dimensions
        static constexpr int buttonHeightS = 24;
        static constexpr int buttonHeightM = 32;
        static constexpr int buttonHeightL = 40;
        static constexpr int transportBarHeight = 60;    // Logic standard (40-60px)
        static constexpr int statusBarHeight = 24;
        static constexpr int mixerHeight = 200;
        
        // Widths
        static constexpr int browserPanelWidth = 280;
        static constexpr int inspectorPanelWidth = 320;
        static constexpr int masterStripWidth = 80;
        static constexpr int mixerStripWidth = 80;
    };

    //==========================================================================
    // Typography
    //==========================================================================
    
    static juce::Font getFontTitle()    { return juce::FontOptions(20.0f, juce::Font::bold); }
    static juce::Font getFontHeading()  { return juce::FontOptions(16.0f, juce::Font::bold); }
    static juce::Font getFontBody()     { return juce::FontOptions(14.0f); }
    static juce::Font getFontSmall()    { return juce::FontOptions(12.0f); }
    static juce::Font getFontTiny()     { return juce::FontOptions(10.0f); }

    //==========================================================================
    // Component Drawing Overrides
    //==========================================================================
    
    // Buttons
    void drawButtonBackground(juce::Graphics& g,
                            juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;
    
    void drawButtonText(juce::Graphics& g,
                       juce::TextButton& button,
                       bool shouldDrawButtonAsHighlighted,
                       bool shouldDrawButtonAsDown) override;
    
    void drawToggleButton(juce::Graphics& g,
                         juce::ToggleButton& button,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;
    
    // Sliders
    void drawRotarySlider(juce::Graphics& g,
                         int x, int y, int width, int height,
                         float sliderPos,
                         float rotaryStartAngle,
                         float rotaryEndAngle,
                         juce::Slider& slider) override;
    
    void drawLinearSlider(juce::Graphics& g,
                         int x, int y, int width, int height,
                         float sliderPos,
                         float minSliderPos,
                         float maxSliderPos,
                         juce::Slider::SliderStyle style,
                         juce::Slider& slider) override;
    
    // ComboBox
    void drawComboBox(juce::Graphics& g,
                     int width, int height,
                     bool isButtonDown,
                     int buttonX, int buttonY,
                     int buttonW, int buttonH,
                     juce::ComboBox& box) override;
    
    // Labels
    void drawLabel(juce::Graphics& g, juce::Label& label) override;
    
    // Scrollbars
    void drawScrollbar(juce::Graphics& g,
                      juce::ScrollBar& scrollbar,
                      int x, int y, int width, int height,
                      bool isScrollbarVertical,
                      int thumbStartPosition,
                      int thumbSize,
                      bool isMouseOver,
                      bool isMouseDown) override;
    
    // Tab buttons
    void drawTabButton(juce::TabBarButton& button,
                      juce::Graphics& g,
                      bool isMouseOver,
                      bool isMouseDown) override;

private:
    //==========================================================================
    // Helper methods
    //==========================================================================
    
    /**
     * @brief Draw rounded rectangle with optional gradient
     */
    void drawRoundedRect(juce::Graphics& g,
                        const juce::Rectangle<float>& bounds,
                        float cornerSize,
                        const juce::Colour& fillColour,
                        const juce::Colour& strokeColour = juce::Colour(),
                        float strokeWidth = 0.0f);
    
    /**
     * @brief Get button color based on state
     */
    juce::Colour getButtonColour(juce::Button& button,
                                bool isHighlighted,
                                bool isDown);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithLookAndFeel)
};

} // namespace zenith

