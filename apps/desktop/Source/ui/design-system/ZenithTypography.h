#pragma once
#include "FontManager.h"

namespace zenith {
class ZenithTypography {
public:
    static juce::Font getHeaderFont() { 
        return design::typography::getJuceFont(24.0f, design::FontWeight::Bold);
    }
    
    static juce::Font getBodyFont() { 
        return design::typography::getJuceFont(14.0f, design::FontWeight::Regular);
    }

    static juce::Font getMonoFont(float size) {
        return design::typography::getJuceMonoFont(size);
    }
};
} // namespace zenith
