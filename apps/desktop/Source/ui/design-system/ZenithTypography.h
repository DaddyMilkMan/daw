#pragma once
#include <juce_graphics/juce_graphics.h>

namespace zenith {
class ZenithTypography {
public:
    static juce::Font getHeaderFont() { return juce::Font(24.0f, juce::Font::bold); }
    static juce::Font getBodyFont() { return juce::Font(14.0f); }
};
}
