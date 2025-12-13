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
#include <include/core/SkColor.h>
#include <include/core/SkFont.h>
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
// Primary Accents - "Electric Dreams"
inline SkColor CYAN =
    0xFF00F0FF; // Electric Blue/Cyan (Slightly warmer than pure Cyan)
inline SkColor MAGENTA = 0xFFFF00D4;    // Hot Pink/Magenta
inline SkColor NEON_GREEN = 0xFF00FF9D; // Spring Green (Modern Mint)
inline SkColor VIOLET = 0xFF7000FF;     // Deep Violet

// Semantic/Status Colors
inline SkColor AMBER = 0xFFFFAB00; // Warm Warning
inline SkColor RED = 0xFFFF453A;   // Soft Red (Apple style)
inline SkColor GREEN = 0xFF32D74B; // Soft Green
inline SkColor BLUE = 0xFF0A84FF;  // iOS Blue

// Backgrounds - "Onyx & Slate" (Rich, deep greys, not voids)
inline SkColor BG_DARKEST = 0xFF0D0D11; // Base/Window Background (Deep Slate)
inline SkColor BG_DARKER = 0xFF141419;  // Panel Background (Subtle separation)
inline SkColor BG_DARK = 0xFF1C1C24;    // Surface/Component Background
inline SkColor BG_MEDIUM = 0xFF25252D;  // Hover Surface
inline SkColor BG_LIGHT = 0xFF2F2F3D;   // Active/Selected Surface

// Text - "High Legibility"
inline SkColor TEXT_PRIMARY = 0xFFF2F2F7;   // Off-white for less eye strain
inline SkColor TEXT_SECONDARY = 0xFFA1A1AA; // Zinc-400 equivalent
inline SkColor TEXT_TERTIARY = 0xFF71717A;  // Zinc-500 equivalent

// Borders & Dividers
inline SkColor BORDER_DEFAULT = 0x1FFFFFFF; // Very subtle white overlay
inline SkColor BORDER_FOCUS = CYAN;
inline SkColor BORDER_SUBTLE = 0x0FFFFFFF; // Ultra subtle
inline SkColor BORDER_STRONG = 0x33FFFFFF; // Visible separation

// Glassmorphism System
inline SkColor GLASS_HIGHLIGHT = 0x1AFFFFFF; // Top edge highlight
inline SkColor GLASS_SHADOW = 0x40000000;    // Drop shadow
inline SkColor GLASS_HOVER = 0x0DFFFFFF;     // White overlay for hover
inline SkColor GLASS_10 = 0x1AFFFFFF; // 10% white (alias for legacy code)

// Text (Additional)
inline SkColor TEXT_DISABLED = 0xFF52525B; // Disabled text (Zinc-600)

// Helper to reset
inline void resetToDefault() {
  CYAN = 0xFF00F0FF;
  MAGENTA = 0xFFFF00D4;
  NEON_GREEN = 0xFF00FF9D;
  // ... (Full reset logic implied)
}
} // namespace colors

// ============================================================================
// THEME MANAGER
// ============================================================================

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

  void saveTheme(const juce::String &name);
  void loadTheme(const juce::String &name);
  void deleteTheme(const juce::String &name);

  juce::StringArray getAvailableThemes() const;

  // Apply current colors to ZenithDesignSystem::Colors
  void applyTheme(const Theme &theme);

private:
  ThemeManager() = default;

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

// Border Radii - "Soft Modern"
constexpr float RADIUS_XS = 2.0f;
constexpr float RADIUS_SM = 4.0f;
constexpr float RADIUS_MD = 8.0f;      // Standard components
constexpr float RADIUS_LG = 12.0f;     // Panels/Containers
constexpr float RADIUS_XL = 16.0f;     // Floating windows
constexpr float RADIUS_FULL = 9999.0f; // Pills/Circles
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
};

} // namespace design
} // namespace zenith
