/*
  ==============================================================================

    ZenithButton.h
    Created: 2025-12-14
    Author:  Zenith DAW Team

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../ZenithLookAndFeel.h"

namespace zenith {

class ZenithButton : public juce::TextButton {
public:
    ZenithButton(const juce::String& buttonName) : juce::TextButton(buttonName) {
        // Ensure we use the custom LookAndFeel if available globally, 
        // otherwise we can set it explicitly in the constructor or parent.
    }
    
    ~ZenithButton() override = default;

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsMouseOver, bool shouldDrawButtonAsDown) override {
        getLookAndFeel().drawButtonBackground(g, *this, findColour(getToggleState() ? buttonOnColourId : buttonColourId),
                                             shouldDrawButtonAsMouseOver, shouldDrawButtonAsDown);
        getLookAndFeel().drawButtonText(g, *this, shouldDrawButtonAsMouseOver, shouldDrawButtonAsDown);
    }
};

} // namespace zenith
