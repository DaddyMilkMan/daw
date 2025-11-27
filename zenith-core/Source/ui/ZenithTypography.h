/**
 * @file ZenithTypography.h
 * @brief Logic Pro style typography system
 *
 * Provides consistent font management across the DAW
 */

#pragma once

#include <JuceHeader.h>

namespace zenith {

class ZenithTypography
{
public:
    // Font families (Logic Pro uses SF Pro Display / Helvetica Neue)
    static juce::Font getMainFont(float size = 12.0f, bool bold = false)
    {
        #ifdef __APPLE__
            return juce::Font("SF Pro Display", size, bold ? juce::Font::bold : juce::Font::plain);
        #else
            // Fallback to Helvetica Neue or Arial
            auto font = juce::Font("Helvetica Neue", size, bold ? juce::Font::bold : juce::Font::plain);
            if (font.getTypefaceName() == "Helvetica Neue")
                return font;
            return juce::Font("Arial", size, bold ? juce::Font::bold : juce::Font::plain);
        #endif
    }
    
    // Monospace font for LCD displays and code
    static juce::Font getMonospaceFont(float size = 12.0f)
    {
        #ifdef __APPLE__
            return juce::Font("SF Mono", size, juce::Font::plain);
        #else
            auto font = juce::Font("Menlo", size, juce::Font::plain);
            if (font.getTypefaceName() == "Menlo")
                return font;
                
            font = juce::Font("Consolas", size, juce::Font::plain);
            if (font.getTypefaceName() == "Consolas")
                return font;
                
            return juce::Font(juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain);
        #endif
    }
    
    // Standard UI sizes (Logic Pro)
    static constexpr float sizeSmall = 10.0f;      // Labels, secondary text
    static constexpr float sizeRegular = 11.0f;    // Default UI text
    static constexpr float sizeMedium = 12.0f;     // Track names, buttons
    static constexpr float sizeLarge = 24.0f;      // LCD display
    
    // Preset fonts for common use cases
    static juce::Font getTrackNameFont()
    {
        return getMainFont(sizeMedium, true);  // 12pt bold
    }
    
    static juce::Font getButtonFont()
    {
        return getMainFont(sizeRegular, true);  // 11pt bold
    }
    
    static juce::Font getLabelFont()
    {
        return getMainFont(sizeSmall, false);  // 10pt regular
    }
    
    static juce::Font getLCDFont()
    {
        return getMonospaceFont(sizeLarge);  // 24pt monospace
    }
    
    static juce::Font getValueFont()
    {
        return getMonospaceFont(sizeSmall);  // 10pt monospace for values
    }
};

} // namespace zenith
