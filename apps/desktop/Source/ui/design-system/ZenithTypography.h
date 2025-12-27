#pragma once
#include "FontManager.h"

namespace zenith {
class ZenithTypography {
public:
    static juce::Font getHeaderFont() { 
        return design::FontManager::getInstance().getDisplayFont(24.0f).getTypeface()->getBounds().isEmpty() ? 
               juce::Font(24.0f, juce::Font::bold) : 
               juce::Font(juce::FontOptions(24.0f).withStyle("Bold")); // Actually we want Inter
    }
    
    static juce::Font getBodyFont() { 
        return juce::Font(14.0f); 
    }
};
} // namespace zenith
