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
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_graphics/juce_graphics.h>
#include <vector>

namespace zenith {
namespace design {

// ============================================================================
// COLOR PALETTE - "Neon Noir" (Mutable for Theming)
// ============================================================================

namespace colors {
    // Primary Accents
    inline SkColor CYAN = 0xFF00FFFF;           // Primary accent
    inline SkColor MAGENTA = 0xFFFF00FF;        // Secondary accent
    inline SkColor NEON_GREEN = 0xFF00FF64;     // Active states
    
    // Status Colors
    inline SkColor AMBER = 0xFFFFC800;          // Warning
    inline SkColor RED = 0xFFFF3232;            // Danger/Error
    inline SkColor BLUE = 0xFF0080FF;           // Info
    
    // Backgrounds (Dark to Darker)
    inline SkColor BG_DARKEST = 0xFF0A0A0F;     // Deepest background
    inline SkColor BG_DARKER = 0xFF0F0F14;      // Panel backgrounds
    inline SkColor BG_DARK = 0xFF141419;        // Component backgrounds
    inline SkColor BG_MEDIUM = 0xFF1A1A23;      // Hover states
    inline SkColor BG_LIGHT = 0xFF20202D;       // Active states
    
    // Text
    inline SkColor TEXT_PRIMARY = 0xFFFFFFFF;   // 100% white
    inline SkColor TEXT_SECONDARY = 0xCCFFFFFF; // 80% white
    
    // Borders
    inline SkColor BORDER_DEFAULT = 0x33FFFFFF; // 20% white
    inline SkColor BORDER_FOCUS = CYAN;         // Cyan for focus
    inline SkColor BORDER_SUBTLE = 0x1AFFFFFF;  // 10% white - subtle dividers
    inline SkColor BORDER_STRONG = 0x66FFFFFF;  // 40% white - emphasized borders
    
    // Additional text colors
    inline SkColor TEXT_DISABLED = 0x66FFFFFF;  // 40% white for disabled text
    
    // Glassmorphism
    inline SkColor GLASS_10 = 0x1AFFFFFF;       // 10% white glass effect
    
    // Helper to reset to default "Neon Noir"
    inline void resetToDefault() {
        CYAN = 0xFF00FFFF;
        MAGENTA = 0xFFFF00FF;
        NEON_GREEN = 0xFF00FF64;
        BG_DARKEST = 0xFF0A0A0F;
        BG_DARKER = 0xFF0F0F14;
        BG_DARK = 0xFF141419;
    }
}

// ============================================================================
// THEME MANAGER
// ============================================================================

/**
 * @class ThemeManager
 * @brief Manages color themes with real-time updates and persistence
 * 
 * Features:
 * - Load/save themes to JSON files
 * - Real-time color updates with listener notification
 * - Default "Neon Noir" theme
 */
class ThemeManager : public juce::ChangeBroadcaster {
public:
    static ThemeManager& getInstance() {
        static ThemeManager instance;
        return instance;
    }
    
    /**
     * @struct Theme
     * @brief Complete theme definition with all color values
     */
    struct Theme {
        juce::String name = "Neon Noir";
        
        // Primary Accents
        SkColor primary = 0xFF00FFFF;       // Cyan
        SkColor secondary = 0xFFFF00FF;     // Magenta
        SkColor accent = 0xFF00FF64;        // Neon Green
        
        // Status Colors
        SkColor warning = 0xFFFFC800;       // Amber
        SkColor danger = 0xFFFF3232;        // Red
        SkColor info = 0xFF0080FF;          // Blue
        
        // Backgrounds
        SkColor bgDarkest = 0xFF0A0A0F;
        SkColor bgDarker = 0xFF0F0F14;
        SkColor bgDark = 0xFF141419;
        SkColor bgMedium = 0xFF1A1A23;
        SkColor bgLight = 0xFF20202D;
        
        // Text
        SkColor textPrimary = 0xFFFFFFFF;
        SkColor textSecondary = 0xCCFFFFFF;
        SkColor textDisabled = 0x66FFFFFF;
        
        // Convert to map for serialization
        std::map<juce::String, uint32_t> toMap() const {
            return {
                {"primary", primary},
                {"secondary", secondary},
                {"accent", accent},
                {"warning", warning},
                {"danger", danger},
                {"info", info},
                {"bgDarkest", bgDarkest},
                {"bgDarker", bgDarker},
                {"bgDark", bgDark},
                {"bgMedium", bgMedium},
                {"bgLight", bgLight},
                {"textPrimary", textPrimary},
                {"textSecondary", textSecondary},
                {"textDisabled", textDisabled}
            };
        }
        
        // Load from map
        void fromMap(const std::map<juce::String, uint32_t>& map) {
            auto get = [&](const juce::String& key, SkColor defaultVal) {
                auto it = map.find(key);
                return it != map.end() ? it->second : defaultVal;
            };
            primary = get("primary", primary);
            secondary = get("secondary", secondary);
            accent = get("accent", accent);
            warning = get("warning", warning);
            danger = get("danger", danger);
            info = get("info", info);
            bgDarkest = get("bgDarkest", bgDarkest);
            bgDarker = get("bgDarker", bgDarker);
            bgDark = get("bgDark", bgDark);
            bgMedium = get("bgMedium", bgMedium);
            bgLight = get("bgLight", bgLight);
            textPrimary = get("textPrimary", textPrimary);
            textSecondary = get("textSecondary", textSecondary);
            textDisabled = get("textDisabled", textDisabled);
        }
    };
    
    //==========================================================================
    // Theme Operations
    //==========================================================================
    
    void saveTheme(const juce::String& name);
    void loadTheme(const juce::String& name);
    void deleteTheme(const juce::String& name);
    juce::StringArray getAvailableThemes() const;
    
    /** Apply a theme, updating all global colors */
    void applyTheme(const Theme& theme);
    
    /** Get the current active theme */
    const Theme& getCurrentTheme() const { return currentTheme_; }
    
    /** Set a specific color and notify listeners */
    void setColor(const juce::String& colorName, SkColor color);
    
    /** Get a specific color by name */
    SkColor getColor(const juce::String& colorName) const;
    
    /** Reset to default "Neon Noir" theme */
    void resetToDefault();
    
private:
    ThemeManager() {
        // Initialize with default theme
        currentTheme_ = Theme();
        applyTheme(currentTheme_);
    }
    
    Theme currentTheme_;
    
    juce::File getThemeDir() const {
        auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("ZenithDAW/Themes");
        if (!dir.exists()) dir.createDirectory();
        return dir;
    }
};

// ============================================================================
// LAYOUT MANAGER
// ============================================================================

class LayoutManager {
public:
    static LayoutManager& getInstance() {
        static LayoutManager instance;
        return instance;
    }
    
    struct PanelState {
        juce::String id;
        juce::Rectangle<float> relativeBounds; // 0.0-1.0 relative to window
        bool isVisible = true;
        int zOrder = 0;
    };
    
    void setPanelState(const juce::String& panelId, const PanelState& state);
    PanelState getPanelState(const juce::String& panelId) const;
    
    void saveLayout(const juce::String& name);
    void loadLayout(const juce::String& name);
    
    bool isEditModeEnabled() const { return editMode_; }
    void setEditModeEnabled(bool enabled) { editMode_ = enabled; }
    
private:
    LayoutManager() = default;
    std::map<juce::String, PanelState> panels_;
    bool editMode_ = false;
    
    juce::File getLayoutDir() const {
        auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("ZenithDAW/Layouts");
        if (!dir.exists()) dir.createDirectory();
        return dir;
    }
};

// ============================================================================
// SPACING SYSTEM - "The Grid"
// ============================================================================
// ... (keep rest of file)

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

// Interpolate (Lerp)
inline float interpolate(float a, float b, float t) {
    return a + (b - a) * t;
}

// Interpolate Color
inline SkColor interpolateColor(SkColor c1, SkColor c2, float t) {
    float invT = 1.0f - t;
    int a = (int)(SkColorGetA(c1) * invT + SkColorGetA(c2) * t);
    int r = (int)(SkColorGetR(c1) * invT + SkColorGetR(c2) * t);
    int g = (int)(SkColorGetG(c1) * invT + SkColorGetG(c2) * t);
    int b = (int)(SkColorGetB(c1) * invT + SkColorGetB(c2) * t);
    return SkColorSetARGB(a, r, g, b);
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
        
        // Accessors for glow intensity
        static float getGlowIntensity() { return glowIntensity; }
        static void setGlowIntensity(float intensity) { 
            glowIntensity = std::clamp(intensity, 0.0f, 2.0f); 
        }
    };

} // namespace design
} // namespace zenith
