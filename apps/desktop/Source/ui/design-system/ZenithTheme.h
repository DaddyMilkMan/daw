/**
 * @file ZenithTheme.h
 * @brief Modern theme system for Zenith DAW
 * @author Fixed by Claude - December 2025
 *
 * Professional design token system with proper hierarchy, spacing, and modern
 * aesthetics.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ZenithDesignSystem.h"

namespace zenith {

/**
 * @class ZenithTheme
 * @brief Modern design system with proper visual hierarchy
 */
class ZenithTheme {
public:
  //==========================================================================
  // Modern Color System - Layered Backgrounds
  // Refactored to static inline for dynamic runtime modification (God Mode)
  //==========================================================================
  struct Colors {
    // Background layers (deepest to most elevated)
    static inline juce::Colour bg_00 = juce::Colour(0xff0a0a0a);
    static inline juce::Colour bg_01 = juce::Colour(0xff121212);
    static inline juce::Colour bg_02 = juce::Colour(0xff1a1a1a);
    static inline juce::Colour bg_03 = juce::Colour(0xff242424);
    static inline juce::Colour bg_04 = juce::Colour(0xff2e2e2e);

    // Borders with proper opacity for depth
    static inline juce::Colour border_subtle = juce::Colours::white.withAlpha(0.06f);
    static inline juce::Colour border_default = juce::Colours::white.withAlpha(0.12f);
    static inline juce::Colour border_strong = juce::Colours::white.withAlpha(0.20f);
    static inline juce::Colour border_focus = juce::Colour(0xff3b82f6);

    // Text hierarchy (proper contrast ratios)
    static inline juce::Colour text_primary = juce::Colours::white.withAlpha(0.95f);
    static inline juce::Colour text_secondary = juce::Colours::white.withAlpha(0.60f);
    static inline juce::Colour text_tertiary = juce::Colours::white.withAlpha(0.35f);
    static inline juce::Colour text_inverse = juce::Colours::black.withAlpha(0.90f);

    // Primary accent
    static inline juce::Colour accent_primary = juce::Colour(0xff3b82f6);
    static inline juce::Colour accent_secondary = juce::Colour(0xff8b5cf6);
    static inline juce::Colour accent_hover = juce::Colour(0xff60a5fa);
    static inline juce::Colour accent_pressed = juce::Colour(0xff2563eb);
    static inline juce::Colour accent_subtle = juce::Colour(0xff3b82f6).withAlpha(0.10f);
    static inline juce::Colour hover_overlay = juce::Colours::white.withAlpha(0.08f);

    // Semantic colors
    static inline juce::Colour success = juce::Colour(0xff10b981);
    static inline juce::Colour warning = juce::Colour(0xfff59e0b);
    static inline juce::Colour error = juce::Colour(0xffef4444);
    static inline juce::Colour info = juce::Colour(0xff06b6d4);

    // Audio-specific colors
    static inline juce::Colour waveform_audio = juce::Colour(0xff3b82f6);
    static inline juce::Colour waveform_midi = juce::Colour(0xff8b5cf6);
    static inline juce::Colour automation = juce::Colour(0xffec4899);
    static inline juce::Colour playhead = juce::Colour(0xfff97316);

    // Track colors (HSL-based for consistency)
    static juce::Colour getTrackColor(int index, float saturation = 0.65f,
                                      float brightness = 0.75f);

    // Dynamic Theme Modification
    static void setColor(const juce::String& colorId, const juce::Colour& color);

    // Utility functions
    static juce::Colour withAlpha(const juce::Colour &color, float alpha);
    static juce::Colour lighten(const juce::Colour &color, float amount);
    static juce::Colour darken(const juce::Colour &color, float amount);
  };

  //==========================================================================
  // Typography System
  //==========================================================================
  struct Typography {
    // Font sizes (8px base scale)
    static constexpr float tiny = 10.0f; // Labels, hints
    static constexpr float sm = 12.0f;   // Secondary text
    static constexpr float body = 14.0f; // Body text (default)
    static constexpr float heading = 16.0f; // Section headers
    static constexpr float large = 20.0f;   // Large headers
    static constexpr float display = 28.0f; // Display text

    // Line heights (1.5x for readability)
    static constexpr float lineHeightTight = 1.2f;
    static constexpr float lineHeightNormal = 1.5f;
    static constexpr float lineHeightLoose = 1.8f;

    // Font weights
    enum class Weight { Regular, Medium, Bold };

    // Helper functions
    static juce::Font getFont(float size, Weight weight = Weight::Regular);
    static juce::Font getTinyFont(Weight weight = Weight::Regular);
    static juce::Font getSmallFont(Weight weight = Weight::Regular);
    static juce::Font getBodyFont(Weight weight = Weight::Regular);
    static juce::Font getHeadingFont(Weight weight = Weight::Bold);
    static juce::Font getLargeFont(Weight weight = Weight::Bold);
    static juce::Font getDisplayFont(Weight weight = Weight::Bold);
  };

  //==========================================================================
  // Spacing System (4px base grid)
  //==========================================================================
  struct Spacing {
    static inline int xs = 4;    // Extra small
    static inline int sm = 8;    // Small
    static inline int md = 16;   // Medium (default)
    static inline int lg = 24;   // Large
    static inline int xl = 32;   // Extra large
    static inline int xxl = 48;  // 2x Extra large
    static inline int xxxl = 64; // 3x Extra large

    // Component-specific spacing
    static inline int trackHeight = 64;       // Minimum track height
    static inline int trackHeaderWidth = 240; // Track header panel width
    static inline int mixerWidth = 80;        // Mixer channel width
    static inline int sidebarWidth = 240;     // Sidebar panel width
    static inline int toolbarHeight = 48;     // Toolbar/transport height
    static inline int statusBarHeight = 24;   // Status bar at bottom
  };

  // Dynamic Layout Modification
  static void setSpacing(const juce::String& key, int value);

  //==========================================================================
  // Border Radius System
  //==========================================================================
  struct Radius {
    static constexpr float none = design::dimensions::RADIUS_NONE;
    static constexpr float sm = design::dimensions::RADIUS_SM;      // Buttons, small controls
    static constexpr float md = design::dimensions::RADIUS_SM;      // Cards, panels
    static constexpr float lg = design::dimensions::RADIUS_LG;      // Modals, large panels
    static constexpr float xl = design::dimensions::RADIUS_LG;     // Special elements
    static constexpr float full = design::dimensions::RADIUS_FULL; // Pills, circular
  };

  //==========================================================================
  // Shadow System (for depth/elevation)
  //==========================================================================
  struct Shadows {
    static constexpr float elevation_0 = 0.0f;  // No shadow
    static constexpr float elevation_1 = 0.05f; // Subtle depth
    static constexpr float elevation_2 = 0.10f; // Buttons, cards
    static constexpr float elevation_3 = 0.15f; // Dropdowns, menus
    static constexpr float elevation_4 = 0.20f; // Modals, dialogs

    static void drawShadow(juce::Graphics &g, juce::Rectangle<float> bounds,
                           float elevation, float radius = 0.0f);
    static void drawInnerShadow(juce::Graphics &g,
                                juce::Rectangle<float> bounds,
                                float radius = 0.0f);
  };

  //==========================================================================
  // Animation Timing
  //==========================================================================
  struct Animation {
    static constexpr int fast = 100;     // Quick transitions
    static constexpr int normal = 200;   // Default transitions
    static constexpr int slow = 300;     // Deliberate transitions
    static constexpr int verySlow = 500; // Dramatic transitions

    // Easing curves (for future animation system)
    enum class Curve { Linear, EaseIn, EaseOut, EaseInOut };
  };

  //==========================================================================
  // Component Dimensions
  //==========================================================================
  struct Components {
    static constexpr int buttonHeightSm = 28;
    static constexpr int buttonHeightMd = 36;
    static constexpr int buttonHeightLg = 44;
    static constexpr int buttonMinWidth = 80;
    static constexpr int inputHeight = 36;
    static constexpr int knobSize = 64;
    static constexpr int faderWidth = 32;
    static constexpr int faderHeight = 120;
    static constexpr int iconSm = 16;
    static constexpr int iconMd = 20;
    static constexpr int iconLg = 24;
    static constexpr int iconXl = 32;
  };

  //==========================================================================
  // Theme Modes
  //==========================================================================
  enum class Mode {
    Standard,    // Default dark theme
    OLED,        // Pure black for OLED displays
    HighContrast // Accessibility mode
  };

  static void setMode(Mode mode);
  static Mode getMode();

private:
  static Mode currentMode_;
};

} // namespace zenith