/**
 * @file ZenithTheme.h
 * @brief Centralized theme system for Zenith DAW
 * @author Marcus "The Craftsman" Rodriguez - Operation Polish
 *
 * Consolidates all color, typography, and styling constants
 * into a single, manageable theme system.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

/**
 * @class ZenithTheme
 * @brief Central theme configuration for entire application
 */
class ZenithTheme {
public:
    //==========================================================================
    // Color Palette
    //==========================================================================
    struct Colors {
        // Backgrounds
        static const juce::Colour background;        // #121212 - Main background
        static const juce::Colour backgroundPanel;   // #1e1e1e - Panel background
        static const juce::Colour backgroundElevated;// #2a2a2a - Elevated surfaces
        
        // Borders
        static const juce::Colour border;            // #333333 - Borders
        static const juce::Colour borderSubtle;      // #444444 - Subtle borders
        
        // Text
        static const juce::Colour textPrimary;       // #ffffff - Primary text
        static const juce::Colour textSecondary;     // #b0b0b0 - Secondary text
        static const juce::Colour textMuted;         // #808080 - Muted text
        
        // Accents
        static const juce::Colour accent;            // #00d4aa - Cyan accent
        static const juce::Colour accentHover;       // #00ffcc - Hover state
        static const juce::Colour accentPressed;     // #00a088 - Pressed state
        
        // Semantic colors
        static const juce::Colour success;           // #4ade80 - Success
        static const juce::Colour warning;           // #fbbf24 - Warning
        static const juce::Colour error;             // #f87171 - Error
        static const juce::Colour info;              // #60a5fa - Info
        
        // Audio-specific
        static const juce::Colour waveform;          // #4a9eff - Audio waveform
        static const juce::Colour midiNote;          // #ff9944 - MIDI notes
        static const juce::Colour automation;        // #a855f7 - Automation curves
    };
    
    //==========================================================================
    // Typography
    //==========================================================================
    struct Typography {
        // Font sizes (in pixels)
        static constexpr float tiny = 10.0f;
        static constexpr float small = 12.0f;
        static constexpr float body = 14.0f;
        static constexpr float heading = 16.0f;
        static constexpr float large = 20.0f;
        static constexpr float display = 24.0f;
        
        // Weights (use JUCE font styles)
        static juce::Font getTinyFont();
        static juce::Font getSmallFont();
        static juce::Font getBodyFont();
        static juce::Font getHeadingFont();
        static juce::Font getLargeFont();
        static juce::Font getDisplayFont();
    };
    
    //==========================================================================
    // Spacing & Layout
    //==========================================================================
    struct Layout {
        static constexpr int paddingTiny = 2;
        static constexpr int paddingSmall = 4;
        static constexpr int paddingMedium = 8;
        static constexpr int paddingLarge = 16;
        static constexpr int paddingXLarge = 24;
        
        static constexpr int borderRadiusSmall = 2;
        static constexpr int borderRadiusMedium = 4;
        static constexpr int borderRadiusLarge = 8;
        
        static constexpr float borderWidthThin = 1.0f;
        static constexpr float borderWidthMedium = 1.5f;
        static constexpr float borderWidthThick = 2.0f;
    };
    
    //==========================================================================
    // Component Dimensions
    //==========================================================================
    struct Components {
        static constexpr int knobSize = 60;
        static constexpr int sliderHeight = 20;
        static constexpr int buttonHeight = 32;
        static constexpr int trackHeaderWidth = 200;
        static constexpr int mixerChannelWidth = 80;
        static constexpr int transportBarHeight = 60;
    };
    
    //==========================================================================
    // Theme Modes
    //==========================================================================
    enum class Mode {
        Standard,  // Default dark theme
        OLED,      // Pure black for OLED displays
        HighContrast // Accessibility mode
    };
    
    static void setMode(Mode mode);
    static Mode getMode();
    
private:
    static Mode currentMode_;
};

} // namespace zenith
