#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {
class ZenithLookAndFeel : public juce::LookAndFeel_V4 {
public:
    ZenithLookAndFeel();
    
    static ZenithLookAndFeel& getInstance();
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, const float rotaryStartAngle,
                          const float rotaryEndAngle, juce::Slider& slider) override;
                          
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override;
                          
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    struct Colors {
        static juce::Colour background;
        static juce::Colour panel;
        static juce::Colour backgroundPanel; 
        static juce::Colour border;          
        static juce::Colour textPrimary;
        static juce::Colour textSecondary;
        static juce::Colour accent;
        static juce::Colour accentGlow;
        
        static void setOledMode(bool enabled);
    };

    struct Spacing {
        static constexpr int xs = 4;
        static constexpr int s = 8;
        static constexpr int m = 16;
        static constexpr int l = 24;
        static constexpr int xl = 32;
    };

    static juce::Font getFontSmall() { return juce::FontOptions(12.0f); }
    static juce::Font getFontMedium() { return juce::FontOptions(14.0f); }
    static juce::Font getFontLarge() { return juce::FontOptions(18.0f); }
};
}
