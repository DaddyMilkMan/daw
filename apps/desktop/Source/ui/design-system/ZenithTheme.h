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

namespace zenith {

/**
 * @class ZenithTheme
 * @brief Modern design system with proper visual hierarchy
 */
class ZenithTheme {
public:
  //==========================================================================
  // Modern Color System - Layered Backgrounds
  //==========================================================================
  struct Colors {
    // Background layers (deepest to most elevated)
    static const juce::Colour bg_00; // #050505 - Deepest layer
    static const juce::Colour bg_01; // #121212 - Canvas/main background
    static const juce::Colour bg_02; // #1a1a1a - Panels
    static const juce::Colour bg_03; // #242424 - Elevated surfaces
    static const juce::Colour bg_04; // #2e2e2e - Highest elevation

    // Borders with proper opacity for depth
    static const juce::Colour
        border_subtle; // White 6% - Barely visible dividers
    static const juce::Colour border_default; // White 12% - Standard borders
    static const juce::Colour border_strong;  // White 20% - Emphasized borders
    static const juce::Colour border_focus;   // Accent color for focus states

    // Text hierarchy (proper contrast ratios)
    static const juce::Colour text_primary;   // White 95% - Main text
    static const juce::Colour text_secondary; // White 60% - Secondary text
    static const juce::Colour text_tertiary;  // White 35% - Disabled/hint text
    static const juce::Colour text_inverse; // Black 90% - On accent backgrounds

    // Primary accent (professional blue instead of garish cyan)
    static const juce::Colour accent_primary; // #00f3ff - Electric Cyan
    static const juce::Colour accent_hover;   // #60a5fa - Hover state
    static const juce::Colour accent_pressed; // #2563eb - Pressed/active state
    static const juce::Colour
        accent_subtle; // Accent with 10% opacity - Backgrounds

    // Semantic colors (status indicators)
    static const juce::Colour success; // #10b981 - Success states
    static const juce::Colour warning; // #f59e0b - Warning states
    static const juce::Colour error;   // #ef4444 - Error states
    static const juce::Colour info;    // #06b6d4 - Info states

    // Audio-specific colors (adjusted for better harmony)
    static const juce::Colour waveform_audio; // #3b82f6 - Audio waveforms
    static const juce::Colour waveform_midi;  // #8b5cf6 - MIDI notes
    static const juce::Colour automation;     // #ec4899 - Automation curves
    static const juce::Colour playhead;       // #f97316 - Playhead indicator

    // Track colors (HSL-based for consistency)
    static juce::Colour getTrackColor(int index, float saturation = 0.65f,
                                      float brightness = 0.75f);

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
    static constexpr float sm = 12.0f;   // Secondary text (renamed from 'small'
                                         // due to Windows macro conflict)
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
    static constexpr int xs = 4;    // Extra small
    static constexpr int sm = 8;    // Small
    static constexpr int md = 16;   // Medium (default)
    static constexpr int lg = 24;   // Large
    static constexpr int xl = 32;   // Extra large
    static constexpr int xxl = 48;  // 2x Extra large
    static constexpr int xxxl = 64; // 3x Extra large

    // Component-specific spacing
    static constexpr int trackHeight = 64;       // Minimum track height
    static constexpr int trackHeaderWidth = 240; // Track header panel width
    static constexpr int mixerWidth = 80;        // Mixer channel width
    static constexpr int sidebarWidth = 240;     // Sidebar panel width
    static constexpr int toolbarHeight = 48;     // Toolbar/transport height
    static constexpr int statusBarHeight = 24;   // Status bar at bottom
  };

  //==========================================================================
  // Border Radius System
  //==========================================================================
  struct Radius {
    static constexpr float none = 0.0f;
    static constexpr float sm = 4.0f;      // Buttons, small controls
    static constexpr float md = 6.0f;      // Cards, panels
    static constexpr float lg = 8.0f;      // Modals, large panels
    static constexpr float xl = 12.0f;     // Special elements
    static constexpr float full = 9999.0f; // Pills, circular
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
    // Button sizes
    static constexpr int buttonHeightSm = 28;
    static constexpr int buttonHeightMd = 36;
    static constexpr int buttonHeightLg = 44;
    static constexpr int buttonMinWidth = 80;

    // Input controls
    static constexpr int inputHeight = 36;
    static constexpr int knobSize = 64;
    static constexpr int faderWidth = 32;
    static constexpr int faderHeight = 120;

    // Icons
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
