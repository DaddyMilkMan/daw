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

#include "ZenithTheme.h"
#include <include/core/SkColor.h>
#include <juce_graphics/juce_graphics.h>

namespace zenith {
namespace design {

// ============================================================================
// COLOR CONVERSION UTILITIES
// ============================================================================

/**
 * Convert JUCE Colour to Skia SkColor.
 * Both use ARGB format, so this is a direct conversion.
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
// UNIFIED COLOR ACCESSORS (Skia format, sourced from ZenithTheme)
// ============================================================================
// These provide a single source of truth using ZenithTheme as the canonical
// color definitions, with automatic conversion to SkColor for Skia rendering.

namespace unified {

// Background layers
inline SkColor bg_00() { return toSkColor(ZenithTheme::Colors::bg_00); }
inline SkColor bg_01() { return toSkColor(ZenithTheme::Colors::bg_01); }
inline SkColor bg_02() { return toSkColor(ZenithTheme::Colors::bg_02); }
inline SkColor bg_03() { return toSkColor(ZenithTheme::Colors::bg_03); }
inline SkColor bg_04() { return toSkColor(ZenithTheme::Colors::bg_04); }

// Borders
inline SkColor border_subtle() { return toSkColor(ZenithTheme::Colors::border_subtle); }
inline SkColor border_default() { return toSkColor(ZenithTheme::Colors::border_default); }
inline SkColor border_strong() { return toSkColor(ZenithTheme::Colors::border_strong); }
inline SkColor border_focus() { return toSkColor(ZenithTheme::Colors::border_focus); }

// Text hierarchy
inline SkColor text_primary() { return toSkColor(ZenithTheme::Colors::text_primary); }
inline SkColor text_secondary() { return toSkColor(ZenithTheme::Colors::text_secondary); }
inline SkColor text_tertiary() { return toSkColor(ZenithTheme::Colors::text_tertiary); }
inline SkColor text_inverse() { return toSkColor(ZenithTheme::Colors::text_inverse); }

// Accent colors
inline SkColor accent_primary() { return toSkColor(ZenithTheme::Colors::accent_primary); }
inline SkColor accent_secondary() { return toSkColor(ZenithTheme::Colors::accent_secondary); }
inline SkColor accent_hover() { return toSkColor(ZenithTheme::Colors::accent_hover); }
inline SkColor accent_pressed() { return toSkColor(ZenithTheme::Colors::accent_pressed); }
inline SkColor accent_subtle() { return toSkColor(ZenithTheme::Colors::accent_subtle); }
inline SkColor hover_overlay() { return toSkColor(ZenithTheme::Colors::hover_overlay); }

// Semantic colors
inline SkColor success() { return toSkColor(ZenithTheme::Colors::success); }
inline SkColor warning() { return toSkColor(ZenithTheme::Colors::warning); }
inline SkColor error() { return toSkColor(ZenithTheme::Colors::error); }
inline SkColor info() { return toSkColor(ZenithTheme::Colors::info); }

// Audio-specific colors
inline SkColor waveform_audio() { return toSkColor(ZenithTheme::Colors::waveform_audio); }
inline SkColor waveform_midi() { return toSkColor(ZenithTheme::Colors::waveform_midi); }
inline SkColor automation() { return toSkColor(ZenithTheme::Colors::automation); }
inline SkColor playhead() { return toSkColor(ZenithTheme::Colors::playhead); }

// Track colors (dynamic)
inline SkColor getTrackColor(int index, float saturation = 0.65f, float brightness = 0.75f) {
    return toSkColor(ZenithTheme::Colors::getTrackColor(index, saturation, brightness));
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
    // Map Neon Noir colors to modern ZenithTheme equivalents
    inline SkColor CYAN() { return unified::accent_primary(); }         // Was electric blue
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
