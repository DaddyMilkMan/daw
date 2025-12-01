/*
  ==============================================================================

    ZenithDesignSystem.h
    Created: 2025-11-30
    Authors: Leo "Lil Bit" Rossi & Yuki Tanaka

    The complete Neon Noir design system for Zenith DAW.
    "If it doesn't glow, it doesn't go!" - Leo
    "Every pixel has a purpose." - Yuki

  ==============================================================================
*/

#pragma once

#include <include/core/SkColor.h>
#include <include/core/SkFont.h>

namespace zenith {
namespace design {

// ============================================================================
// COLOR PALETTE - "Neon Noir"
// ============================================================================

namespace colors {
    // Primary Accents
    constexpr SkColor CYAN = 0xFF00FFFF;           // Primary accent
    constexpr SkColor MAGENTA = 0xFFFF00FF;        // Secondary accent
    constexpr SkColor NEON_GREEN = 0xFF00FF64;     // Active states
    
    // Status Colors
    constexpr SkColor AMBER = 0xFFFFC800;          // Warning
    constexpr SkColor RED = 0xFFFF3232;            // Danger/Error
    constexpr SkColor BLUE = 0xFF0080FF;           // Info
    
    // Backgrounds (Dark to Darker)
    constexpr SkColor BG_DARKEST = 0xFF0A0A0F;     // Deepest background
    constexpr SkColor BG_DARKER = 0xFF0F0F14;      // Panel backgrounds
    constexpr SkColor BG_DARK = 0xFF141419;        // Component backgrounds
    constexpr SkColor BG_MEDIUM = 0xFF1A1A23;      // Hover states
    constexpr SkColor BG_LIGHT = 0xFF20202D;       // Active states
    
    // Glass/Transparency
    constexpr SkColor GLASS_10 = 0x1AFFFFFF;       // 10% white
    constexpr SkColor GLASS_20 = 0x33FFFFFF;       // 20% white
    constexpr SkColor GLASS_30 = 0x4DFFFFFF;       // 30% white
    constexpr SkColor GLASS_40 = 0x66FFFFFF;       // 40% white
    
    // Text
    constexpr SkColor TEXT_PRIMARY = 0xFFFFFFFF;   // 100% white
    constexpr SkColor TEXT_SECONDARY = 0xCCFFFFFF; // 80% white
    constexpr SkColor TEXT_TERTIARY = 0x99FFFFFF;  // 60% white
    constexpr SkColor TEXT_DISABLED = 0x66FFFFFF;  // 40% white
    
    // Borders
    constexpr SkColor BORDER_SUBTLE = 0x1AFFFFFF;  // 10% white
    constexpr SkColor BORDER_DEFAULT = 0x33FFFFFF; // 20% white
    constexpr SkColor BORDER_STRONG = 0x4DFFFFFF;  // 30% white
    constexpr SkColor BORDER_FOCUS = CYAN;         // Cyan for focus
}

// ============================================================================
// SPACING SYSTEM - "The Grid"
// ============================================================================

namespace spacing {
    constexpr float XS = 4.0f;      // Tiny gaps
    constexpr float SM = 8.0f;      // Small gaps
    constexpr float MD = 16.0f;     // Standard gaps
    constexpr float LG = 24.0f;     // Large gaps
    constexpr float XL = 32.0f;     // Extra large gaps
    constexpr float XXL = 48.0f;    // Huge gaps
    
    // Component-specific
    constexpr float PANEL_PADDING = MD;
    constexpr float COMPONENT_GAP = SM;
    constexpr float SECTION_GAP = LG;
}

// ============================================================================
// TYPOGRAPHY - "Readable & Beautiful"
// ============================================================================

namespace typography {
    // Font Sizes
    constexpr float FONT_XS = 10.0f;    // Labels, hints
    constexpr float FONT_SM = 12.0f;    // Secondary text
    constexpr float FONT_MD = 14.0f;    // Body text
    constexpr float FONT_LG = 16.0f;    // Headings
    constexpr float FONT_XL = 20.0f;    // Large headings
    constexpr float FONT_XXL = 24.0f;   // Display text
    
    // Font Weights (if custom fonts support it)
    constexpr int WEIGHT_LIGHT = 300;
    constexpr int WEIGHT_REGULAR = 400;
    constexpr int WEIGHT_MEDIUM = 500;
    constexpr int WEIGHT_BOLD = 700;
    
    // Line Heights (multipliers)
    constexpr float LINE_HEIGHT_TIGHT = 1.2f;
    constexpr float LINE_HEIGHT_NORMAL = 1.5f;
    constexpr float LINE_HEIGHT_RELAXED = 1.8f;
}

// ============================================================================
// DIMENSIONS - "Standard Sizes"
// ============================================================================

namespace dimensions {
    // Component Heights
    constexpr float BUTTON_HEIGHT = 32.0f;
    constexpr float BUTTON_HEIGHT_SM = 24.0f;
    constexpr float BUTTON_HEIGHT_LG = 40.0f;
    
    constexpr float INPUT_HEIGHT = 32.0f;
    constexpr float KNOB_SIZE = 64.0f;
    constexpr float KNOB_SIZE_SM = 48.0f;
    constexpr float KNOB_SIZE_LG = 80.0f;
    
    // Panel Sizes
    constexpr float TRANSPORT_BAR_HEIGHT = 60.0f;
    constexpr float LEFT_SIDEBAR_WIDTH = 280.0f;
    constexpr float RIGHT_SIDEBAR_WIDTH = 320.0f;
    constexpr float BOTTOM_PANEL_HEIGHT = 200.0f;
    
    // Minimum Sizes
    constexpr float MIN_PANEL_WIDTH = 200.0f;
    constexpr float MIN_PANEL_HEIGHT = 100.0f;
    
    // Border Radii - TIGHTENED FOR PRO LOOK
    constexpr float RADIUS_SM = 2.0f;       // Was 4.0f
    constexpr float RADIUS_MD = 4.0f;       // Was 8.0f
    constexpr float RADIUS_LG = 8.0f;       // Was 12.0f
    constexpr float RADIUS_FULL = 9999.0f;  // Fully rounded
}

// ============================================================================
// EFFECTS - "The Glow System"
// ============================================================================

namespace effects {
    // Glow/Blur Radii
    constexpr float GLOW_SUBTLE = 2.0f;     // Hover
    constexpr float GLOW_MEDIUM = 4.0f;     // Active
    constexpr float GLOW_STRONG = 6.0f;     // Focus
    constexpr float GLOW_INTENSE = 8.0f;    // Error/Warning
    constexpr float BLUR_GLASS = 20.0f;     // Glassmorphism
    
    // Opacity Levels
    constexpr float OPACITY_SUBTLE = 0.2f;
    constexpr float OPACITY_MEDIUM = 0.4f;
    constexpr float OPACITY_STRONG = 0.6f;
    constexpr float OPACITY_INTENSE = 0.8f;
    
    // Shadow Offsets
    constexpr float SHADOW_OFFSET_SM = 2.0f;
    constexpr float SHADOW_OFFSET_MD = 4.0f;
    constexpr float SHADOW_OFFSET_LG = 8.0f;
}

// ============================================================================
// ANIMATION - "Smooth & Buttery"
// ============================================================================

namespace animation {
    // Durations (milliseconds)
    constexpr int DURATION_INSTANT = 0;
    constexpr int DURATION_FAST = 100;
    constexpr int DURATION_NORMAL = 200;
    constexpr int DURATION_SLOW = 300;
    constexpr int DURATION_SLOWER = 500;
    
    // Easing curves (for reference, actual implementation in animation system)
    // - ease-in: slow start, fast end
    // - ease-out: fast start, slow end
    // - ease-in-out: slow start and end
    // - spring: physics-based bounce
    
    // Frame Rate Targets
    constexpr int FPS_TARGET = 60;
    constexpr int FPS_HIGH = 120;
    constexpr float FRAME_TIME_60FPS = 16.67f;  // milliseconds
    constexpr float FRAME_TIME_120FPS = 8.33f;  // milliseconds
}

// ============================================================================
// Z-INDEX - "Layering System"
// ============================================================================

namespace zindex {
    constexpr int BACKGROUND = 0;
    constexpr int PANEL = 10;
    constexpr int COMPONENT = 20;
    constexpr int CONTROL = 30;
    constexpr int OVERLAY = 40;
    constexpr int MODAL = 50;
    constexpr int TOOLTIP = 60;
    constexpr int NOTIFICATION = 70;
}

// HELPER FUNCTIONS
// Create color with alpha
inline SkColor withAlpha(SkColor color, float alpha) {
    return SkColorSetARGB(
        static_cast<U8CPU>(alpha * 255.0f),
        SkColorGetR(color),
        SkColorGetG(color),
        SkColorGetB(color)
    );
}

// Lighten color
inline SkColor lighten(SkColor color, float amount) {
    return SkColorSetARGB(
        SkColorGetA(color),
        std::min(255, static_cast<int>(SkColorGetR(color) * (1.0f + amount))),
        std::min(255, static_cast<int>(SkColorGetG(color) * (1.0f + amount))),
        std::min(255, static_cast<int>(SkColorGetB(color) * (1.0f + amount)))
    );
}

// Darken color
inline SkColor darken(SkColor color, float amount) {
    return SkColorSetARGB(
        SkColorGetA(color),
        static_cast<int>(SkColorGetR(color) * (1.0f - amount)),
        static_cast<int>(SkColorGetG(color) * (1.0f - amount)),
        static_cast<int>(SkColorGetB(color) * (1.0f - amount))
    );
}
    // ============================================================================
    // GLOBAL SETTINGS (Karen Fixes)
    // ============================================================================
    
    struct Settings {
        static float glowIntensity; // 0.0 to 2.0 (default 1.0)
        static float uiScale;       // 0.5 to 2.0 (default 1.0)
        
        // Theme Management
        enum class Theme {
            NeonNoir,
            OLEDBlack,
            Classic
        };
        static Theme currentTheme;
    };

} // namespace design
} // namespace zenith
