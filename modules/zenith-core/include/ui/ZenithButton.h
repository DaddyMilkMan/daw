/**
 * @file ZenithButton.h
 * @brief Button with style support for both JUCE and Skia rendering
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include "../../src/ui/skia/ZenithUIComponents.h"
#else

namespace zenith {

// JUCE fallback button (only used when Skia is disabled)
class ZenithButton : public juce::TextButton
{
public:
    enum ButtonStyle
    {
        Primary,
        Secondary,
        Danger,
        Warning,
        Success
    };

    ZenithButton() = default;
    explicit ZenithButton(const juce::String& buttonText) : juce::TextButton(buttonText) {}

    void setButtonStyle(ButtonStyle style)
    {
        currentStyle_ = style;

        // Apply basic colors based on style
        switch (style)
        {
            case Primary:
                setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0066cc));
                setColour(juce::TextButton::textColourOffId, juce::Colours::white);
                break;
            case Secondary:
                setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a4a4a));
                setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
                break;
            case Danger:
                setColour(juce::TextButton::buttonColourId, juce::Colour(0xffcc3333));
                setColour(juce::TextButton::textColourOffId, juce::Colours::white);
                break;
            case Warning:
                setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff9900));
                setColour(juce::TextButton::textColourOffId, juce::Colours::white);
                break;
            case Success:
                setColour(juce::TextButton::buttonColourId, juce::Colour(0xff33aa33));
                setColour(juce::TextButton::textColourOffId, juce::Colours::white);
                break;
        }

        repaint();
    }

    ButtonStyle getButtonStyle() const { return currentStyle_; }

private:
    ButtonStyle currentStyle_ = Secondary;
};

} // namespace zenith

#endif // !ZENITH_USE_SKIA
