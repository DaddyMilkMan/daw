/*
  ==============================================================================

    ColorBridge.h
    Created: 2025-12-25
    Author:  Zenith DAW

    Unified color access layer bridging JUCE Colours and Skia SkColors.
    
    The design system has two color backends:
    - ZenithTheme::Colors (juce::Colour) - Used by JUCE components
    - design::colors (SkColor) - Used by Skia rendering
    
    This bridge provides:
    1. Conversion utilities between color formats
    2. Unified accessors that return the appropriate format
    3. A single source of truth for color values

  ==============================================================================
*/

#pragma once

#include "ZenithDesignSystem.h"
#include <core/SkColor.h>
#include <juce_graphics/juce_graphics.h>

namespace zenith {
namespace design {

    // ============================================================================
    // COLOR CONVERSION UTILITIES
    // ============================================================================
    
    /**
     * Convert JUCE Colour to Skia SkColor.
     */
    inline SkColor toSkColor(const juce::Colour& c) {
        return SkColorSetARGB(c.getAlpha(), c.getRed(), c.getGreen(), c.getBlue());
    }
    
    /**
     * Convert Skia SkColor to JUCE Colour.
     */
    inline juce::Colour toJuceColour(SkColor c) {
        return juce::Colour((juce::uint8)SkColorGetR(c), 
                            (juce::uint8)SkColorGetG(c), 
                            (juce::uint8)SkColorGetB(c), 
                            (juce::uint8)SkColorGetA(c));
    }
    
    /**
     * Convert JUCE Colour to SkColor with alpha modification.
     */
    inline SkColor toSkColor(const juce::Colour& c, float alpha) {
        return SkColorSetARGB(
            static_cast<U8CPU>(alpha * 255.0f),
            c.getRed(),
            c.getGreen(),
            c.getBlue()
        );
    }
    
    // ============================================================================
    // UNIFIED COLOR ACCESSORS (Skia format, sourced from ZenithDesignSystem)
    // ============================================================================
    
    namespace unified {
    
    // Background layers
    inline SkColor bg_00() { return design::colors::BG_00; }
    inline SkColor bg_01() { return design::colors::BG_01; }
    inline SkColor bg_02() { return design::colors::BG_02; }
    inline SkColor bg_03() { return design::colors::BG_03; }
    inline SkColor bg_04() { return design::colors::BG_04; }
    
    // Borders
    inline SkColor border_subtle() { return design::colors::BORDER_SUBTLE; }
    inline SkColor border_default() { return design::colors::BORDER_DEFAULT; }
    inline SkColor border_strong() { return design::colors::BORDER_STRONG; }
    inline SkColor border_focus() { return design::colors::BORDER_FOCUS; }
    
    // Text hierarchy
    inline SkColor text_primary() { return design::colors::TEXT_PRIMARY; }
    inline SkColor text_secondary() { return design::colors::TEXT_SECONDARY; }
    inline SkColor text_tertiary() { return design::colors::TEXT_TERTIARY; }
    inline SkColor text_inverse() { return design::colors::TEXT_INVERSE; }
    
    // Accent colors
    inline SkColor accent_primary() { return design::colors::ACCENT_PRIMARY; }
    inline SkColor accent_secondary() { return design::colors::ACCENT_SECONDARY; }
    inline SkColor accent_hover() { return design::colors::CYAN; } // Default hover
    inline SkColor accent_pressed() { return design::colors::CYAN; } // Default pressed
    inline SkColor accent_subtle() { return design::withAlpha(design::colors::ACCENT_PRIMARY, 0.1f); }
    inline SkColor hover_overlay() { return design::colors::GLASS_HOVER; }
    
    // Semantic colors
    inline SkColor success() { return design::colors::SUCCESS; }
    inline SkColor warning() { return design::colors::WARNING; }
    inline SkColor error() { return design::colors::DANGER; }
    inline SkColor info() { return design::colors::INFO; }
    
    // Audio-specific colors
    inline SkColor waveform_audio() { return design::colors::WAVEFORM_AUDIO; }
    inline SkColor waveform_midi() { return design::colors::WAVEFORM_MIDI; }
    inline SkColor automation() { return design::colors::AUTOMATION; }
    inline SkColor playhead() { return design::colors::PLAYHEAD; }
    
    // Track colors (dynamic)
    inline SkColor getTrackColor(int index, float saturation = 0.65f, float brightness = 0.75f) {
        // Simple hue rotation based on index
        float hue = std::fmod(index * 0.618033988749895f, 1.0f);
        SkScalar hsv[3] = {hue * 360.0f, saturation, brightness};
        return SkHSVToColor(hsv);
    }

// Alpha/brightness modifiers
inline SkColor withAlpha(SkColor color, float alpha) {
    return SkColorSetARGB(
        static_cast<U8CPU>(alpha * 255.0f),
        SkColorGetR(color),
        SkColorGetG(color),
        SkColorGetB(color)
    );
}

} // namespace unified

// ============================================================================
// LEGACY COMPATIBILITY MAPPINGS
// ============================================================================
// Map old design::colors names to unified namespace for gradual migration.
// Components can migrate from design::colors::CYAN to design::unified::accent_primary()

namespace compat {
    // Map legacy accent names to modern ZenithTheme equivalents
    inline SkColor CYAN() { return unified::accent_primary(); }         // Legacy primary accent
    inline SkColor ACCENT_PRIMARY() { return unified::accent_primary(); }
    inline SkColor MAGENTA() { return unified::accent_secondary(); }    // Purple/violet
    
    // Background mappings
    inline SkColor BG_DARKEST() { return unified::bg_00(); }
    inline SkColor BG_DARKER() { return unified::bg_01(); }
    inline SkColor BG_DARK() { return unified::bg_02(); }
    inline SkColor BG_MEDIUM() { return unified::bg_03(); }
    inline SkColor BG_LIGHT() { return unified::bg_04(); }
    
    // Text mappings
    inline SkColor TEXT_PRIMARY() { return unified::text_primary(); }
    inline SkColor TEXT_SECONDARY() { return unified::text_secondary(); }
    inline SkColor TEXT_TERTIARY() { return unified::text_tertiary(); }
    
    // Border mappings
    inline SkColor BORDER_DEFAULT() { return unified::border_default(); }
    inline SkColor BORDER_FOCUS() { return unified::border_focus(); }
    inline SkColor BORDER_SUBTLE() { return unified::border_subtle(); }
    
    // Semantic mappings
    inline SkColor SUCCESS() { return unified::success(); }
    inline SkColor DANGER() { return unified::error(); }
    inline SkColor WARNING() { return unified::warning(); }
}

} // namespace design
} // namespace zenith
