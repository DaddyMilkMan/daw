/*
  ==============================================================================

    ZenithTypography.h
    Created: 2025-12-08
    Author: UI Overhaul - Professional Typography System

    Centralized font management for Zenith DAW.
    
    Design Decisions:
    - Use SkTypeface::MakeDefault() for cross-platform compatibility
    - Use SkFont with proper anti-aliasing settings
    - Define semantic text styles for consistent UI

  ==============================================================================
*/

#pragma once

#include <core/SkFont.h>
#include <core/SkTypeface.h>
#include <juce_core/juce_core.h>

namespace zenith {

// ============================================================================
// TEXT STYLE PRESETS (Semantic sizing)
// ============================================================================

namespace TextStyle {
    // Display sizes (large headers, time displays)
    constexpr float DISPLAY_XL = 32.0f;
    constexpr float DISPLAY_LG = 28.0f;
    constexpr float DISPLAY_MD = 24.0f;
    
    // Heading sizes
    constexpr float HEADING_LG = 20.0f;
    constexpr float HEADING_MD = 18.0f;
    constexpr float HEADING_SM = 16.0f;
    
    // Body sizes
    constexpr float BODY_LG = 15.0f;
    constexpr float BODY_MD = 14.0f;
    constexpr float BODY_SM = 13.0f;
    
    // Caption/label sizes
    constexpr float CAPTION = 12.0f;
    constexpr float TINY = 10.0f;
    constexpr float MICRO = 9.0f;
    
    // Specific contexts
    constexpr float TRANSPORT_TIME = 22.0f;  // Transport bar time display
    constexpr float MIXER_LABEL = 11.0f;     // Mixer channel labels
    constexpr float KNOB_VALUE = 12.0f;      // Knob value readout
    constexpr float BUTTON_TEXT = 13.0f;     // Button labels
    constexpr float MENU_ITEM = 14.0f;       // Menu items
    constexpr float TOOLTIP = 12.0f;         // Tooltips
}

// ============================================================================
// TYPOGRAPHY MANAGER
// ============================================================================

class ZenithTypography {
public:
    /**
     * Initialize the typography system.
     * Call once at application startup.
     */
    static void initialize() {
        if (initialized_) return;
        
        // Use default typeface - works across all platforms
        // SkTypeface::MakeDefault() was removed in newer Skia, use nullptr which creates default
        defaultTypeface_ = nullptr;
        
        initialized_ = true;
        DBG("ZenithTypography: Initialized with default typeface");
    }
    
    /**
     * Get a configured SkFont for rendering.
     * @param size Font size in pixels
     * @param isBold Use bold weight
     */
    static SkFont getFont(float size, bool isBold = false) {
        if (!initialized_) {
            initialize();
        }
        
        SkFont font(defaultTypeface_, scaledSize(size));
        
        // High-quality text rendering settings
        font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
        font.setSubpixel(true);
        font.setHinting(SkFontHinting::kSlight);
        
        // Emulate bold with skew if needed
        if (isBold) {
            font.setEmbolden(true);
        }
        
        return font;
    }
    
    /**
     * Get mono-spaced font (for time displays, values, etc.)
     * Uses a slightly different configuration for tabular figures
     */
    static SkFont getMonoFont(float size, bool isBold = false) {
        if (!initialized_) {
            initialize();
        }
        
        SkFont font(defaultTypeface_, scaledSize(size));
        font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
        font.setSubpixel(true);
        font.setHinting(SkFontHinting::kSlight);
        
        if (isBold) {
            font.setEmbolden(true);
        }
        
        return font;
    }
    
    /**
     * Quick access to common font configurations.
     */
    static SkFont displayFont(float size = TextStyle::DISPLAY_MD) {
        return getFont(size, true);
    }
    
    static SkFont headingFont(float size = TextStyle::HEADING_MD) {
        return getFont(size, true);
    }
    
    static SkFont bodyFont(float size = TextStyle::BODY_MD) {
        return getFont(size, false);
    }
    
    static SkFont captionFont(float size = TextStyle::CAPTION) {
        return getFont(size, false);
    }
    
    static SkFont labelFont(float size = TextStyle::CAPTION) {
        return getFont(size, false);
    }
    
    static SkFont buttonFont(float size = TextStyle::BUTTON_TEXT) {
        return getFont(size, false);
    }
    
    static SkFont transportFont() {
        return getMonoFont(TextStyle::TRANSPORT_TIME, false);
    }
    
    static SkFont valueFont(float size = TextStyle::KNOB_VALUE) {
        return getMonoFont(size, false);
    }
    
    /**
     * Get global UI scale factor.
     */
    static float getUIScale() { return uiScale_; }
    static void setUIScale(float scale) { uiScale_ = juce::jlimit(0.5f, 2.0f, scale); }
    
    /**
     * Apply UI scale to a font size.
     */
    static float scaledSize(float size) { return size * uiScale_; }
    
private:
    ZenithTypography() = default;
    
    static inline sk_sp<SkTypeface> defaultTypeface_ = nullptr;
    static inline bool initialized_ = false;
    static inline float uiScale_ = 1.0f;
};

} // namespace zenith
