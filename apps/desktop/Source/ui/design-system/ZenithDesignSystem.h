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
#include "FontManager.h"
#include <core/SkBlurTypes.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <vector>

namespace zenith {
namespace design {

// ============================================================================
// COLOR PALETTE - "Neon Noir" (Mutable for Theming)
// ============================================================================

namespace colors {
// ============================================================================
// PRIMARY PALETTE (Single Source of Truth)
// ============================================================================

// Brand Colors
inline SkColor CYAN = 0xFF00F0FF;        // Electric Blue (Primary Brand)
inline SkColor MAGENTA = 0xFFFF00D4;     // Hot Pink (Secondary Brand)
inline SkColor VIOLET = 0xFF7000FF;      // Deep Violet
inline SkColor ORANGE = 0xFFFF8800;      // Orange
inline SkColor NEON_GREEN = 0xFF00FF9D;  // Spring Green
inline SkColor NEON_PINK = 0xFFFF1493;   // Deep Pink
inline SkColor NEON_RED = 0xFFFF073A;    // Neon Red
inline SkColor NEON_YELLOW = 0xFFFFFF00; // Yellow
inline SkColor NEON_PURPLE = 0xFFAA00FF; // Purple
inline SkColor CYAN_DARK = 0xFF008888;   // Dark Cyan

// Functional Palette
inline SkColor BLUE = 0xFF3B82F6;        // Standard Blue (Info/Action)
inline SkColor GREEN = 0xFF10B981;       // Success/Safe
inline SkColor YELLOW = 0xFFF59E0B;      // Warning/Caution
inline SkColor RED = 0xFFEF4444;         // Error/Danger
inline SkColor PINK = 0xFFEC4899;        // Automation
inline SkColor AMBER = 0xFFFFAB00;       // Warm Warning

// Background Layers (Deepest to Elevated)
inline SkColor BG_00 = 0xFF0A0A0A;       // Deepest/App Background
inline SkColor BG_01 = 0xFF121212;       // Canvas/Main
inline SkColor BG_02 = 0xFF1A1A1A;       // Panels
inline SkColor BG_03 = 0xFF242424;       // Elevated Surfaces
inline SkColor BG_04 = 0xFF2E2E2E;       // Highest Elevation (Modals/Popups)

// Legacy Background Aliases (Deprecated)
inline SkColor BG_DARKEST = BG_00;
inline SkColor BG_DARKER = BG_01;
inline SkColor BG_DARK = BG_02;
inline SkColor BG_MEDIUM = BG_03;
inline SkColor BG_LIGHT = BG_04;

// Semantic Aliases
inline SkColor ACCENT_PRIMARY = CYAN;
inline SkColor ACCENT_SECONDARY = MAGENTA;
inline SkColor NEON_CYAN = CYAN;         // Deprecated Alias
inline SkColor SURFACE_BASE = BG_01;
inline SkColor SURFACE_ELEVATED = BG_02;
inline SkColor SUCCESS = GREEN;
inline SkColor DANGER = RED;
inline SkColor WARNING = AMBER;
inline SkColor INFO = BLUE;

// Text Hierarchy
inline SkColor TEXT_PRIMARY = 0xFFF2F2F7;    // High Emphasis (95%)
inline SkColor TEXT_SECONDARY = 0xFFA1A1AA;  // Medium Emphasis (60%)
inline SkColor TEXT_TERTIARY = 0xFF71717A;   // Disabled/Hints (35%)
inline SkColor TEXT_DISABLED = 0xFF52525B;   // Disabled text
inline SkColor TEXT_INVERSE = 0xFF111111;    // Text on Accent

// Borders & Dividers
inline SkColor BORDER_SUBTLE = 0x0FFFFFFF;   // 6% White
inline SkColor BORDER_DEFAULT = 0x1FFFFFFF;  // 12% White
inline SkColor BORDER_STRONG = 0x33FFFFFF;   // 20% White
inline SkColor BORDER_FOCUS = CYAN;          // Focus Ring
inline SkColor BORDER_GREETING = 0x1AFFFFFF; // Legacy

// Glassmorphism System
inline SkColor GLASS_HIGHLIGHT = 0x1AFFFFFF; // Top edge highlight
inline SkColor GLASS_SHADOW = 0x66000000;    // Drop shadow
inline SkColor GLASS_HOVER = 0x0DFFFFFF;     // White overlay for hover
inline SkColor GLASS_10 = 0x1AFFFFFF;        // Generic glass

// Audio Visualization
inline SkColor WAVEFORM_AUDIO = BLUE;
inline SkColor WAVEFORM_MIDI = MAGENTA;
inline SkColor AUTOMATION = PINK;
inline SkColor PLAYHEAD = ORANGE;

// Helper to reset ALL colors (for runtime theme reload and testing)
inline void resetToDefault() {
  // Brand Colors
  CYAN = 0xFF00F0FF;
  MAGENTA = 0xFFFF00D4;
  VIOLET = 0xFF7000FF;
  ORANGE = 0xFFFF8800;
  NEON_GREEN = 0xFF00FF9D;
  NEON_PINK = 0xFFFF1493;
  NEON_RED = 0xFFFF073A;
  NEON_YELLOW = 0xFFFFFF00;
  NEON_PURPLE = 0xFFAA00FF;
  CYAN_DARK = 0xFF008888;

  // Functional Palette
  BLUE = 0xFF3B82F6;
  GREEN = 0xFF10B981;
  YELLOW = 0xFFF59E0B;
  RED = 0xFFEF4444;
  PINK = 0xFFEC4899;
  AMBER = 0xFFFFAB00;

  // Background Layers
  BG_00 = 0xFF0A0A0A;
  BG_01 = 0xFF121212;
  BG_02 = 0xFF1A1A1A;
  BG_03 = 0xFF242424;
  BG_04 = 0xFF2E2E2E;

  // Legacy Background Aliases
  BG_DARKEST = BG_00;
  BG_DARKER = BG_01;
  BG_DARK = BG_02;
  BG_MEDIUM = BG_03;
  BG_LIGHT = BG_04;

  // Semantic Aliases
  ACCENT_PRIMARY = CYAN;
  ACCENT_SECONDARY = MAGENTA;
  NEON_CYAN = CYAN;
  SURFACE_BASE = BG_01;
  SURFACE_ELEVATED = BG_02;
  SUCCESS = GREEN;
  DANGER = RED;
  WARNING = AMBER;
  INFO = BLUE;

  // Text Hierarchy
  TEXT_PRIMARY = 0xFFF2F2F7;
  TEXT_SECONDARY = 0xFFA1A1AA;
  TEXT_TERTIARY = 0xFF71717A;
  TEXT_DISABLED = 0xFF52525B;
  TEXT_INVERSE = 0xFF111111;

  // Borders & Dividers
  BORDER_SUBTLE = 0x0FFFFFFF;
  BORDER_DEFAULT = 0x1FFFFFFF;
  BORDER_STRONG = 0x33FFFFFF;
  BORDER_FOCUS = CYAN;
  BORDER_GREETING = 0x1AFFFFFF;

  // Glassmorphism System
  GLASS_HIGHLIGHT = 0x1AFFFFFF;
  GLASS_SHADOW = 0x66000000;
  GLASS_HOVER = 0x0DFFFFFF;
  GLASS_10 = 0x1AFFFFFF;

  // Audio Visualization
  WAVEFORM_AUDIO = BLUE;
  WAVEFORM_MIDI = MAGENTA;
  AUTOMATION = PINK;
  PLAYHEAD = ORANGE;
}
} // namespace colors

// ============================================================================
// OPACITY TOKENS - Semantic opacity values (eliminates magic floats)
// ============================================================================
// Use these instead of hardcoded values like withAlpha(color, 0.15f)
// Access via: design::opacity::HOVER, design::opacity::ACTIVE, etc.

namespace opacity {
// State opacities (for text, icons, and elements)
constexpr float DISABLED = 0.35f;     // Disabled elements, tertiary text
constexpr float SECONDARY = 0.6f;     // Secondary text, de-emphasized elements
constexpr float TERTIARY = 0.45f;     // Tertiary elements
constexpr float PRIMARY = 0.95f;      // Primary text (near-white, not pure)

// Interaction opacities (for overlays and state changes)
constexpr float HOVER = 0.08f;        // Hover overlay on surfaces
constexpr float ACTIVE = 0.15f;       // Active/pressed overlay
constexpr float FOCUS = 0.20f;        // Focus ring background
constexpr float SELECTED = 0.25f;     // Selected state overlay

// Surface opacities (for glassmorphism and panels)
constexpr float GLASS_SUBTLE = 0.06f; // Subtle glass effect
constexpr float GLASS_MEDIUM = 0.12f; // Medium glass effect
constexpr float GLASS_STRONG = 0.20f; // Strong glass effect
constexpr float GLASS_SOLID = 0.30f;  // Nearly solid glass

// Border opacities
constexpr float BORDER_SUBTLE = 0.06f;   // Barely visible dividers
constexpr float BORDER_DEFAULT = 0.12f;  // Standard borders
constexpr float BORDER_STRONG = 0.20f;   // Emphasized borders
constexpr float BORDER_ACCENT = 0.60f;   // Accent color borders

// Glow opacities
constexpr float GLOW_SUBTLE = 0.20f;  // Subtle glow
constexpr float GLOW_MEDIUM = 0.40f;  // Medium glow
constexpr float GLOW_STRONG = 0.60f;  // Strong glow
constexpr float GLOW_INTENSE = 0.80f; // Intense glow (warnings, errors)

// Shadow opacities
constexpr float SHADOW_LIGHT = 0.10f;   // Light shadow
constexpr float SHADOW_MEDIUM = 0.20f;  // Medium shadow
constexpr float SHADOW_STRONG = 0.40f;  // Strong shadow
} // namespace opacity

// ============================================================================
// ACCESSIBILITY TOKENS - WCAG 2.1 AA Compliance
// ============================================================================
// Reference: https://www.w3.org/WAI/WCAG21/
// Focus indicators: 2.4.7 Focus Visible, 2.4.13 Focus Appearance
// Contrast: 1.4.3 (4.5:1 text), 1.4.11 (3:1 UI components)

namespace accessibility {

// Focus Ring - WCAG 2.4.13 compliant (3:1 contrast, 2px minimum)
constexpr float FOCUS_RING_WIDTH = 2.0f;
constexpr float FOCUS_RING_OFFSET = 2.0f;
inline SkColor FOCUS_RING_COLOR = colors::CYAN;  // 00F0FF on 121212 = 12.5:1 ✅

// Touch Target - WCAG 2.5.5 (44x44px minimum recommended)
constexpr float MIN_TOUCH_TARGET = 44.0f;
constexpr float MIN_TOUCH_TARGET_SM = 24.0f;

// Responsive Breakpoints
constexpr int BREAKPOINT_COMPACT = 400;   // Narrow layout mode
constexpr int BREAKPOINT_NORMAL = 800;    // Standard layout
constexpr int BREAKPOINT_WIDE = 1200;     // Full-width layout

// Luminance calculation (sRGB to relative luminance per WCAG 2.1)
inline float luminance(SkColor c) {
  auto linearize = [](float v) -> float {
    return v <= 0.03928f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f);
  };
  float r = linearize(SkColorGetR(c) / 255.0f);
  float g = linearize(SkColorGetG(c) / 255.0f);
  float b = linearize(SkColorGetB(c) / 255.0f);
  return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

// Contrast ratio calculation per WCAG 2.1
inline float contrastRatio(SkColor fg, SkColor bg) {
  float l1 = luminance(fg);
  float l2 = luminance(bg);
  if (l2 > l1) std::swap(l1, l2);
  return (l1 + 0.05f) / (l2 + 0.05f);
}

// WCAG AA compliance checks
inline bool meetsWCAG_AA_Text(SkColor fg, SkColor bg) {
  return contrastRatio(fg, bg) >= 4.5f;
}

inline bool meetsWCAG_AA_LargeText(SkColor fg, SkColor bg) {
  return contrastRatio(fg, bg) >= 3.0f;
}

inline bool meetsWCAG_AA_UI(SkColor fg, SkColor bg) {
  return contrastRatio(fg, bg) >= 3.0f;
}

} // namespace accessibility

// ============================================================================
// THEME MANAGER
// ============================================================================

/**
 * @brief Theme preset enumeration for built-in themes
 */
enum class ThemePreset {
  Dark,   // Neon Noir - vibrant accents on dark backgrounds
  Darker, // OLED Black - pure black backgrounds for power saving
  Light   // Light mode - inverted palette for daylight use
};

/**
 * @brief Listener interface for theme change notifications
 */
class ThemeListener {
public:
  virtual ~ThemeListener() = default;
  virtual void themeChanged(ThemePreset newTheme) = 0;
};

class ThemeManager : public juce::ChangeBroadcaster {
public:
  static ThemeManager &getInstance() {
    static ThemeManager instance;
    return instance;
  }

  struct Theme {
    juce::String name;
    std::map<juce::String, uint32_t> colors; // name -> ARGB
  };

  // Built-in preset management
  void setActiveTheme(ThemePreset preset);
  ThemePreset getActiveTheme() const { return activePreset_; }

  // Listener management
  void addListener(ThemeListener *listener);
  void removeListener(ThemeListener *listener);

  // Custom theme management
  void saveTheme(const juce::String &name);
  void loadTheme(const juce::String &name);
  void deleteTheme(const juce::String &name);

  juce::StringArray getAvailableThemes() const;

  // Apply current colors to ZenithDesignSystem::Colors
  void applyTheme(const Theme &theme);
  
  // Reset state for testing
  void resetToDefault();

  // Palette accessors for current theme
  struct ThemePalette {
    // Backgrounds
    SkColor bgDarkest;
    SkColor bgDarker;
    SkColor bgDark;
    SkColor bgMedium;
    SkColor bgLight;

    // Accents
    SkColor accentPrimary;
    SkColor accentSecondary;

    // Text
    SkColor textPrimary;
    SkColor textSecondary;
    SkColor textTertiary;

    // Borders
    SkColor borderDefault;
    SkColor borderSubtle;
    SkColor borderFocus;

    // Semantic
    SkColor success;
    SkColor warning;
    SkColor error;
  };

  const ThemePalette &getPalette() const { return currentPalette_; }

private:
  ThemeManager();

  void applyDarkTheme();
  void applyDarkerTheme();
  void applyLightTheme();
  void notifyListeners();

  ThemePreset activePreset_ = ThemePreset::Dark;
  ThemePalette currentPalette_;
  std::vector<ThemeListener *> listeners_;

  juce::File getThemeDir() const {
    auto dir =
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("ZenithDAW/Themes");
    if (!dir.exists())
      dir.createDirectory();
    return dir;
  }
};


// ============================================================================
// LAYOUT MANAGER
// ============================================================================

class LayoutManager {
public:
  static LayoutManager &getInstance() {
    static LayoutManager instance;
    return instance;
  }

  struct PanelState {
    juce::String id;
    juce::Rectangle<float> relativeBounds; // 0.0-1.0 relative to window
    bool isVisible = true;
    int zOrder = 0;
  };

  void setPanelState(const juce::String &panelId, const PanelState &state);
  PanelState getPanelState(const juce::String &panelId) const;

  void saveLayout(const juce::String &name);
  void loadLayout(const juce::String &name);

  bool isEditModeEnabled() const { return editMode_; }
  void setEditModeEnabled(bool enabled) { editMode_ = enabled; }

private:
  LayoutManager() = default;
  std::map<juce::String, PanelState> panels_;
  bool editMode_ = false;

  juce::File getLayoutDir() const {
    auto dir =
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("ZenithDAW/Layouts");
    if (!dir.exists())
      dir.createDirectory();
    return dir;
  }
};

// ============================================================================
// SPACING SYSTEM - "The Grid"
// ============================================================================
// ... (keep rest of file)

namespace spacing {
constexpr float XS = 4.0f;   // Tiny gaps
constexpr float SM = 8.0f;   // Small gaps
constexpr float MD = 16.0f;  // Standard gaps
constexpr float LG = 24.0f;  // Large gaps
constexpr float XL = 32.0f;  // Extra large gaps
constexpr float XXL = 48.0f; // Huge gaps

// Component-specific
constexpr float PANEL_PADDING = MD;
constexpr float COMPONENT_GAP = SM;
constexpr float SECTION_GAP = LG;
} // namespace spacing

// ============================================================================
// TYPOGRAPHY - "Readable & Beautiful"
// ============================================================================

namespace typography {

// ============================================================================
// FONT ACCESS FUNCTIONS
// ============================================================================

/**
 * Get a UI font (Inter) with the specified size and weight.
 * This is the primary function for obtaining fonts throughout the UI.
 * All returned fonts are configured with subpixel antialiasing.
 *
 * @param size   Font size in points
 * @param weight Font weight (defaults to Regular)
 * @return       Configured SkFont ready for rendering
 */
inline SkFont getSkFont(float size, FontWeight weight = FontWeight::Regular) {
  return FontManager::getInstance().getUIFont(size, weight);
}

/**
 * Get a monospace font (JetBrains Mono) for code and fixed-width displays.
 * Ideal for tempo displays, timing readouts, parameter values, and code.
 *
 * @param size   Font size in points
 * @param weight Font weight (defaults to Regular)
 * @return       Configured SkFont ready for rendering
 */
inline SkFont getMonoFont(float size, FontWeight weight = FontWeight::Regular) {
  return FontManager::getInstance().getMonoFont(size, weight);
}

/**
 * Get a display font for large headings and titles.
 * Uses Inter with typically bold weight for maximum impact.
 *
 * @param size   Font size in points
 * @param weight Font weight (defaults to Bold)
 * @return       Configured SkFont ready for rendering
 */
inline SkFont getDisplayFont(float size, FontWeight weight = FontWeight::Bold) {
  return FontManager::getInstance().getDisplayFont(size, weight);
}

// ============================================================================
// JUCE FONT COMPATIBILITY
// ============================================================================

/**
 * Get a juce::Font that matches the design system's Inter font.
 * Used by standard JUCE components that don't use Skia rendering.
 */
inline juce::Font getJuceFont(float size, FontWeight weight = FontWeight::Regular) {
    juce::FontOptions options;
    options = options.withHeight(size);
    options = options.withName("Inter");
    
    if (weight == FontWeight::Bold) options = options.withStyle("Bold");
    else if (weight == FontWeight::SemiBold) options = options.withStyle("SemiBold");
    else if (weight == FontWeight::Medium) options = options.withStyle("Medium");
    
    return juce::Font(options);
}

/**
 * Get a juce::Font for monospace text.
 */
inline juce::Font getJuceMonoFont(float size) {
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain));
}

// ============================================================================
// LEGACY COMPATIBILITY
// ============================================================================

/**
 * Legacy compatibility wrapper - prefer getSkFont(size, weight) instead.
 * This overload is provided for backward compatibility with existing code.
 */
[[deprecated("Use getSkFont(size, weight) instead.")]]
inline SkFont getSkFontWithSize(float size) {
  return zenith::design::typography::getSkFont(size, FontWeight::Regular);
}

// ============================================================================
// FONT SIZE CONSTANTS
// ============================================================================

constexpr float FONT_XS = 10.0f;  // Labels, hints, small captions
constexpr float FONT_SM = 12.0f;  // Secondary text, metadata
constexpr float FONT_MD = 14.0f;  // Body text, default UI text
constexpr float FONT_LG = 16.0f;  // Headings, emphasized text
constexpr float FONT_XL = 20.0f;  // Large headings, section titles
constexpr float FONT_XXL = 24.0f; // Display text, major titles

// ============================================================================
// FONT WEIGHT CONSTANTS (for legacy code)
// ============================================================================

constexpr int WEIGHT_LIGHT = 300;
constexpr int WEIGHT_REGULAR = 400;
constexpr int WEIGHT_MEDIUM = 500;
constexpr int WEIGHT_SEMIBOLD = 600;
constexpr int WEIGHT_BOLD = 700;

// ============================================================================
// LINE HEIGHT MULTIPLIERS
// ============================================================================

constexpr float LINE_HEIGHT_TIGHT = 1.2f;   // Compact layouts
constexpr float LINE_HEIGHT_NORMAL = 1.5f;  // Standard readability
constexpr float LINE_HEIGHT_RELAXED = 1.8f; // Spacious layouts

} // namespace typography

// Expose typography functions to zenith::design namespace
using typography::getDisplayFont;
using typography::getMonoFont;
using typography::getSkFont;

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

// Border Radii - Standardized Two-Tier System
// Small (8px): Buttons, inputs, cards, small components
// Large (16px): Panels, dialogs, modals, large containers
constexpr float RADIUS_NONE = 0.0f;       // Sharp corners
constexpr float RADIUS_SM = 8.0f;         // Standard components (buttons, inputs, cards)
constexpr float RADIUS_LG = 16.0f;        // Large containers (panels, dialogs, modals)
constexpr float RADIUS_FULL = 9999.0f;    // Pills/Circles

// Legacy aliases - kept for backward compatibility, prefer RADIUS_SM/RADIUS_LG
[[deprecated("Use RADIUS_SM (8.0f) instead")]]
constexpr float RADIUS_XS = RADIUS_SM;
[[deprecated("Use RADIUS_SM (8.0f) instead")]]
constexpr float RADIUS_MD = RADIUS_SM;
[[deprecated("Use RADIUS_LG (16.0f) instead")]]
constexpr float RADIUS_XL = RADIUS_LG;

// ===========================================================================
// MIXER DIMENSIONS - Layout constants for mixer channel strips
// ===========================================================================
// Anti-Corner-Cutting: No hardcoded pixel values in component code!
constexpr int MIXER_CHANNEL_WIDTH = 100;   // Standard channel strip width
constexpr int MIXER_MASTER_WIDTH = 140;    // Master channel (wider)
constexpr int MIXER_CHANNEL_SPACING = 4;   // Gap between channels
constexpr int DIVIDER_WIDTH = 2;           // Vertical divider thickness

// ===========================================================================
// TRACK DIMENSIONS - Arranger track sizes
// ===========================================================================
constexpr int TRACK_HEIGHT_MIN = 64;       // Minimum collapsed track
constexpr int TRACK_HEIGHT_DEFAULT = 100;  // Standard track height
constexpr int TRACK_HEIGHT_EXPANDED = 200; // Expanded track with lanes
constexpr int TRACK_HEADER_WIDTH = 200;    // Left-side track header

// ===========================================================================
// ARRANGER DIMENSIONS - Timeline view sizing
// ===========================================================================
constexpr int TIMELINE_RULER_HEIGHT = 24;  // Top time ruler
constexpr int SECTION_TRACK_HEIGHT = 32;   // Arrangement markers lane
constexpr int TEMPO_LANE_HEIGHT = 48;      // BPM envelope lane

} // namespace dimensions

// ============================================================================
// EFFECTS - "The Glow System"
// ============================================================================

namespace effects {
// Glow/Blur Radii
constexpr float GLOW_SUBTLE = 2.0f;  // Hover
constexpr float GLOW_MEDIUM = 4.0f;  // Active
constexpr float GLOW_STRONG = 6.0f;  // Focus
constexpr float GLOW_INTENSE = 8.0f; // Error/Warning
constexpr float BLUR_GLASS = 20.0f;  // Glassmorphism

// Opacity Levels
constexpr float OPACITY_SUBTLE = 0.2f;
constexpr float OPACITY_MEDIUM = 0.4f;
constexpr float OPACITY_STRONG = 0.6f;
constexpr float OPACITY_INTENSE = 0.8f;

// Shadow Offsets
constexpr float SHADOW_OFFSET_SM = 2.0f;
constexpr float SHADOW_OFFSET_MD = 4.0f;
constexpr float SHADOW_OFFSET_LG = 8.0f;
} // namespace effects

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
constexpr float FRAME_TIME_60FPS = 16.67f; // milliseconds
constexpr float FRAME_TIME_120FPS = 8.33f; // milliseconds

} // namespace animation

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
} // namespace zindex

// HELPER FUNCTIONS
// Create color with alpha
inline SkColor withAlpha(SkColor color, float alpha) {
  return SkColorSetARGB(static_cast<U8CPU>(alpha * 255.0f), SkColorGetR(color),
                        SkColorGetG(color), SkColorGetB(color));
}

// Lighten color
inline SkColor lighten(SkColor color, float amount) {
  return SkColorSetARGB(
      SkColorGetA(color),
      std::min(255, static_cast<int>(SkColorGetR(color) * (1.0f + amount))),
      std::min(255, static_cast<int>(SkColorGetG(color) * (1.0f + amount))),
      std::min(255, static_cast<int>(SkColorGetB(color) * (1.0f + amount))));
}

// Darken color
inline SkColor darken(SkColor color, float amount) {
  return SkColorSetARGB(SkColorGetA(color),
                        static_cast<int>(SkColorGetR(color) * (1.0f - amount)),
                        static_cast<int>(SkColorGetG(color) * (1.0f - amount)),
                        static_cast<int>(SkColorGetB(color) * (1.0f - amount)));
}

// Interpolate (Lerp)
inline float interpolate(float a, float b, float t) { return a + (b - a) * t; }

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
// COMPONENT TOKENS - Button
// ============================================================================
// Component-level tokens wrap semantic tokens with DAW-specific accessors.
// This follows Material Design's primitive → semantic → component hierarchy.
// Reference: https://material.io/design/tokens

namespace button {
// Background colors by style (sourced from colors namespace)
inline SkColor getBgPrimary() { return colors::CYAN; }
inline SkColor getBgSecondary() { return colors::BG_02; }
inline SkColor getBgDanger() { return colors::RED; }
inline SkColor getBgSuccess() { return colors::GREEN; }
inline SkColor getBgWarning() { return colors::AMBER; }
inline SkColor getBgGhost() { return withAlpha(colors::BG_02, 0.0f); }

// Text colors
inline SkColor getTextDefault() { return colors::TEXT_PRIMARY; }
inline SkColor getTextOnAccent() { return colors::TEXT_INVERSE; }
inline SkColor getTextDisabled() { return colors::TEXT_TERTIARY; }

// Border colors
inline SkColor getBorderDefault() { return colors::BORDER_DEFAULT; }
inline SkColor getBorderFocus() { return colors::BORDER_FOCUS; }
inline SkColor getBorderGhost() { return colors::BORDER_SUBTLE; }

// Glow colors (match background for consistency)
inline SkColor getGlowPrimary() { return colors::CYAN; }
inline SkColor getGlowDanger() { return colors::RED; }
inline SkColor getGlowSuccess() { return colors::GREEN; }
inline SkColor getGlowWarning() { return colors::AMBER; }
inline SkColor getGlowSecondary() { return colors::VIOLET; }

// State opacities (from opacity namespace)
inline float getHoverOpacity() { return opacity::HOVER; }
inline float getActiveOpacity() { return opacity::ACTIVE; }
inline float getFocusOpacity() { return opacity::FOCUS; }
inline float getDisabledOpacity() { return opacity::DISABLED; }

// Dimensions
inline float getHeightSm() { return 24.0f; }
inline float getHeightMd() { return 32.0f; }
inline float getHeightLg() { return 40.0f; }
inline float getRadiusSm() { return dimensions::RADIUS_SM; }
inline float getRadiusLg() { return dimensions::RADIUS_LG; }
inline float getPaddingX() { return spacing::SM * 1.5f; } // 12px
inline float getPaddingY() { return spacing::XS; }         // 4px

// Typography
inline float getFontSizeSm() { return typography::FONT_XS; }
inline float getFontSizeMd() { return typography::FONT_SM; }
inline float getFontSizeLg() { return typography::FONT_MD; }

// Animation
inline int getHoverDuration() { return animation::DURATION_FAST; }
inline int getPressDuration() { return animation::DURATION_FAST; }
} // namespace button

// ============================================================================
// COMPONENT TOKENS - Slider
// ============================================================================

namespace slider {
// Track colors
inline SkColor getTrackBg() { return colors::BG_01; }
inline SkColor getTrackBorder() { return colors::BORDER_SUBTLE; }
inline SkColor getTrackFillDefault() { return colors::CYAN; }
inline SkColor getTrackFillMuted() { return colors::AMBER; }
inline SkColor getTrackFillSolo() { return colors::NEON_GREEN; }

// Handle colors
inline SkColor getHandleDefault() { return colors::TEXT_PRIMARY; }
inline SkColor getHandleHover() { return colors::CYAN; }
inline SkColor getHandleActive() { return colors::CYAN; }
inline SkColor getHandleDisabled() { return colors::TEXT_TERTIARY; }

// Glow
inline SkColor getGlowDefault() { return colors::CYAN; }
inline float getGlowIntensity() { return effects::GLOW_MEDIUM; }

// Dimensions
inline float getTrackHeightHorizontal() { return 4.0f; }
inline float getTrackWidthVertical() { return 4.0f; }
inline float getHandleSize() { return 16.0f; }
inline float getHandleSizeLg() { return 20.0f; }
inline float getHandleRadius() { return dimensions::RADIUS_FULL; }

// DB Fader specific
inline float getFaderWidth() { return 32.0f; }
inline float getFaderHeight() { return 120.0f; }
inline float getFaderHandleHeight() { return 24.0f; }

// State opacities
inline float getDisabledOpacity() { return opacity::DISABLED; }
} // namespace slider

// ============================================================================
// COMPONENT TOKENS - Panel
// ============================================================================

namespace panel {
// Background colors by elevation (BG_00 = deepest, BG_04 = highest)
inline SkColor getBg0() { return colors::BG_00; } // App background
inline SkColor getBg1() { return colors::BG_01; } // Canvas/main
inline SkColor getBg2() { return colors::BG_02; } // Panels
inline SkColor getBg3() { return colors::BG_03; } // Elevated surfaces
inline SkColor getBg4() { return colors::BG_04; } // Modals/popups

// Border colors
inline SkColor getBorderDefault() { return colors::BORDER_DEFAULT; }
inline SkColor getBorderSubtle() { return colors::BORDER_SUBTLE; }
inline SkColor getBorderStrong() { return colors::BORDER_STRONG; }

// Glassmorphism
inline SkColor getGlassHighlight() { return colors::GLASS_HIGHLIGHT; }
inline SkColor getGlassShadow() { return colors::GLASS_SHADOW; }
inline float getGlassBlur() { return effects::BLUR_GLASS; }

// Header/footer
inline SkColor getHeaderBg() { return colors::BG_03; }
inline SkColor getFooterBg() { return colors::BG_02; }

// Dimensions
inline float getPadding() { return spacing::MD; }
inline float getPaddingSm() { return spacing::SM; }
inline float getPaddingLg() { return spacing::LG; }
inline float getRadius() { return dimensions::RADIUS_LG; }
inline float getRadiusSm() { return dimensions::RADIUS_SM; }

// Header heights
inline float getHeaderHeight() { return 32.0f; }
inline float getToolbarHeight() { return dimensions::TRANSPORT_BAR_HEIGHT; }

// Shadow
inline float getShadowOpacity() { return opacity::SHADOW_MEDIUM; }
inline float getShadowOffset() { return effects::SHADOW_OFFSET_MD; }
} // namespace panel

// ============================================================================
// GLOBAL SETTINGS (Karen Fixes)
// ============================================================================

struct Settings {
  static float glowIntensity; // 0.0 to 2.0 (default 1.0)
  static float uiScale;       // 0.5 to 2.0 (default 1.0)

  // Theme Management
  enum class Theme { NeonNoir, OLEDBlack, Classic };
  static Theme currentTheme;

  // Backdrop Blur Quality (for glassmorphism performance)
  // Off = solid panels, Low = 50% blur, Medium = 75% blur, High = full blur
  enum class BlurQuality { Off, Low, Medium, High };
  static BlurQuality blurQuality;

  // Accessors for glow intensity
  static float getGlowIntensity() { return glowIntensity; }
  static void setGlowIntensity(float intensity) {
    glowIntensity = std::clamp(intensity, 0.0f, 2.0f);
  }

  // Accessors for blur quality
  static BlurQuality getBlurQuality() { return blurQuality; }
  static void setBlurQuality(BlurQuality quality) { blurQuality = quality; }

  // Reduced Motion (Accessibility)
  static bool reducedMotionEnabled;
  static bool isReducedMotionEnabled() { return reducedMotionEnabled; }
  static void setReducedMotionEnabled(bool enabled) { reducedMotionEnabled = enabled; }
};

} // namespace design
} // namespace zenith
