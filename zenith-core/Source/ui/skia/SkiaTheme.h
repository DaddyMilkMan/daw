/**
 * @file SkiaTheme.h
 * @brief Comprehensive theme system for Skia UI with dark/light modes
 *
 * Provides a complete design system with:
 * - Dark and light color schemes
 * - Full 3D depth effects (shadows, highlights, gradients)
 * - Optimized spring physics parameters per component type
 * - GPU-optimized rendering constants
 * - Typography settings for flashy Skia text
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkTypes.h"

#else
#include <cstdint>
#endif

namespace zenith {

#ifndef ZENITH_USE_SKIA
using SkColor = uint32_t;
#endif

/**
 * @enum ThemeMode
 * @brief Theme color mode
 */
enum class ThemeMode { Dark, Light };

/**
 * @struct SpringPhysicsSettings
 * @brief Spring animation parameters optimized per component type
 */
struct SpringPhysicsSettings {
  float stiffness; ///< Spring stiffness coefficient (higher = snappier)
  float damping;   ///< Damping coefficient (higher = less overshoot)
  float fps;       ///< Animation frame rate

  // Common presets
  static SpringPhysicsSettings snappy() {
    return {450.0f, 30.0f, 60.0f};
  } // Fast, minimal overshoot
  static SpringPhysicsSettings smooth() {
    return {350.0f, 25.0f, 60.0f};
  } // Balanced feel
  static SpringPhysicsSettings bouncy() {
    return {300.0f, 18.0f, 60.0f};
  } // More playful
  static SpringPhysicsSettings precise() {
    return {500.0f, 35.0f, 120.0f};
  } // Ultra-responsive, high FPS
};

/**
 * @struct DepthStyle
 * @brief 3D depth rendering parameters
 */
struct DepthStyle {
  float shadowBlur;       ///< Shadow blur radius
  float shadowOffsetY;    ///< Shadow vertical offset
  float shadowOpacity;    ///< Shadow opacity (0-1)
  float highlightOpacity; ///< Top highlight opacity (0-1)
  float innerShadowSize;  ///< Inner shadow/bevel size
  bool useGradients;      ///< Enable gradient fills

  // Preset styles
  static DepthStyle subtle() { return {4.0f, 2.0f, 0.3f, 0.15f, 1.0f, true}; }
  static DepthStyle moderate() { return {8.0f, 3.0f, 0.4f, 0.25f, 2.0f, true}; }
  static DepthStyle dramatic() {
    return {12.0f, 4.0f, 0.5f, 0.35f, 3.0f, true};
  }
};

/**
 * @struct ThemeColors
 * @brief Complete color palette for a theme mode
 */
struct ThemeColors {
  // Neutrals (Backgrounds)
  SkColor bg0; ///< App background (#050608)
  SkColor bg1; ///< Primary panel background (#111418)
  SkColor bg2; ///< Elevated panel / headers (#181C22)
  SkColor bg3; ///< Controls / input fields (#1F242C)

  // Borders
  SkColor borderSubtle; ///< Dividers, grid lines (#2A313A)
  SkColor borderStrong; ///< Active borders, strong dividers (#3A4450)

  // Text
  SkColor textStrong; ///< Primary text, headers (#F8FAFF)
  SkColor textMuted;  ///< Secondary text, labels (#A4ACBA)
  SkColor textSubtle; ///< Disabled text, placeholders (#6C7380)
  SkColor textDanger; ///< Error text (#FF5C5C)

  // Accents
  SkColor accentMain;    ///< Primary brand color (Teal #00D4AA)
  SkColor accentAlt;     ///< Selection / Focus (Blue #4C8DFF)
  SkColor accentRecord;  ///< Record / Armed (Red #FF3B30)
  SkColor accentWarning; ///< Warnings (Yellow #FFC857)

  // Musical Data (Clip Palette)
  SkColor clipDrums;   ///< #FF8A65
  SkColor clipBass;    ///< #FFD54F
  SkColor clipHarmony; ///< #81C784
  SkColor clipLeads;   ///< #64B5F6
  SkColor clipFX;      ///< #BA68C8

  // Legacy mappings (for backward compatibility)
  SkColor background;
  SkColor backgroundSecondary;
  SkColor backgroundTertiary;

  SkColor surfaceDefault;
  SkColor surfaceHover;
  SkColor surfaceActive;
  SkColor surfaceDisabled;

  SkColor primary;
  SkColor primaryHover;
  SkColor primaryActive;

  SkColor success;
  SkColor successHover;

  SkColor danger;
  SkColor dangerHover;

  SkColor warning;
  SkColor warningHover;

  SkColor textPrimary;
  SkColor textSecondary;
  SkColor textDisabled;
  SkColor textOnAccent;

  SkColor border;
  SkColor borderHover;
  SkColor divider;

  SkColor shadowDark;
  SkColor shadowLight;
  SkColor innerShadow;
  SkColor highlight;

  SkColor waveformAudio;
  SkColor waveformMidi;
  SkColor gridLine;
  SkColor playhead;
  SkColor loopRegion;
  SkColor meterGreen;
  SkColor meterYellow;
  SkColor meterRed;
};

/**
 * @class SkiaTheme
 * @brief Comprehensive theme system for Skia-rendered UI
 *
 * Provides centralized access to:
 * - Color palettes (dark/light)
 * - Spring physics parameters per component type
 * - 3D depth rendering settings
 * - Typography and text effects
 * - GPU rendering optimizations
 */
class SkiaTheme {
public:
  //==========================================================================
  // Singleton access
  //==========================================================================

  static SkiaTheme &getInstance();

  //==========================================================================
  // Theme mode
  //==========================================================================

  void setThemeMode(ThemeMode mode);
  ThemeMode getThemeMode() const { return currentMode_; }

  const ThemeColors &getColors() const { return colors_; }

  //==========================================================================
  // Spring physics presets per component type
  //==========================================================================

  SpringPhysicsSettings getButtonPhysics() const { return buttonPhysics_; }
  SpringPhysicsSettings getSliderPhysics() const { return sliderPhysics_; }
  SpringPhysicsSettings getKnobPhysics() const { return knobPhysics_; }
  SpringPhysicsSettings getWaveformPhysics() const { return waveformPhysics_; }
  SpringPhysicsSettings getTimelinePhysics() const { return timelinePhysics_; }

  void setButtonPhysics(const SpringPhysicsSettings &settings) {
    buttonPhysics_ = settings;
  }
  void setSliderPhysics(const SpringPhysicsSettings &settings) {
    sliderPhysics_ = settings;
  }
  void setKnobPhysics(const SpringPhysicsSettings &settings) {
    knobPhysics_ = settings;
  }
  void setWaveformPhysics(const SpringPhysicsSettings &settings) {
    waveformPhysics_ = settings;
  }
  void setTimelinePhysics(const SpringPhysicsSettings &settings) {
    timelinePhysics_ = settings;
  }

  //==========================================================================
  // Depth style
  //==========================================================================

  const DepthStyle &getDepthStyle() const { return depthStyle_; }
  void setDepthStyle(const DepthStyle &style) { depthStyle_ = style; }

  //==========================================================================
  // Typography (for Skia text rendering)
  //==========================================================================

  struct TextStyle {
    float size;
    bool bold;
  };

  struct Typography {
    // Named text styles (POLISH: comprehensive typography system)
    TextStyle title = {16.0f, true};  ///< Large headings, panel titles
    TextStyle header = {14.0f, true}; ///< Section headers
    TextStyle body = {12.0f, false};  ///< Default body text
    TextStyle small = {10.0f, false}; ///< Labels, secondary info
    TextStyle tiny = {8.0f, false};   ///< Timestamps, minimal text
    TextStyle mono = {11.0f, false};  ///< Timecode, values only

    // Legacy sizes (for backward compatibility)
    float baseSize = 14.0f;    ///< Base font size
    float headingSize = 18.0f; ///< Heading size
    float smallSize = 12.0f;   ///< Small text
    float largeSize = 16.0f;   ///< Large text

    bool enableTextGlow = true;   ///< Enable glow effects
    float textGlowRadius = 3.0f;  ///< Glow blur radius
    float textGlowOpacity = 0.6f; ///< Glow opacity

    bool enableTextShadow = true;   ///< Enable drop shadow
    float textShadowOffsetY = 1.0f; ///< Shadow offset
    float textShadowBlur = 2.0f;    ///< Shadow blur
    float textShadowOpacity = 0.5f; ///< Shadow opacity
  };

  const Typography &getTypography() const { return typography_; }
  void setTypography(const Typography &typo) { typography_ = typo; }

  //==========================================================================
  // Interaction Colors (POLISH: unified hover/active/focus)
  //==========================================================================

  struct InteractionColors {
    SkColor hoverOverlay;  ///< Subtle white overlay for hover (low alpha)
    SkColor activeOverlay; ///< Stronger overlay for active/pressed
    SkColor focusBorder;   ///< Focus ring color (accent-based)
  };

  const InteractionColors &getInteraction() const { return interaction_; }
  void setInteraction(const InteractionColors &colors) {
    interaction_ = colors;
  }

  //==========================================================================
  // Selection Style (POLISH: unified selection rendering)
  //==========================================================================

  enum class SelectionMode {
    BorderOnly,    ///< Selection shown via border only
    BorderAndTint, ///< Selection shown via border + background tint
    Highlight      ///< Selection shown via full highlight overlay
  };

  struct SelectionStyle {
    SelectionMode mode = SelectionMode::BorderAndTint;
    SkColor borderColor;       ///< Selection border color (default: accentAlt)
    float borderWidth = 2.0f;  ///< Border width in pixels
    SkColor tintColor;         ///< Background tint for selected items
    float tintOpacity = 0.15f; ///< Tint opacity (0-1)
    float cornerRadius = 4.0f; ///< Corner radius for selection highlight
  };

  const SelectionStyle &getSelectionStyle() const { return selectionStyle_; }
  void setSelectionStyle(const SelectionStyle &style) {
    selectionStyle_ = style;
  }

  //==========================================================================
  // GPU rendering settings
  //==========================================================================

  struct GPUSettings {
    int targetFPS = 60;            ///< Target frame rate
    bool adaptiveFPS = true;       ///< Auto-adjust FPS based on load
    bool prioritizeQuality = true; ///< Visual quality over performance
    int waveformDetailLevel =
        3; ///< Waveform detail (1-5, higher = more detail)
    bool enableAntialiasing = true; ///< MSAA/SSAA
    int msaaSamples = 4;            ///< MSAA sample count (2, 4, 8, 16)
    bool enableMipmaps = true;      ///< Mipmaps for scaled images
  };

  const GPUSettings &getGPUSettings() const { return gpuSettings_; }
  void setGPUSettings(const GPUSettings &settings) { gpuSettings_ = settings; }

  //==========================================================================
  // Helper methods
  //==========================================================================

  /**
   * @brief Create a gradient from two colors
   * @param topColor Top of gradient
   * @param radius Blur radius
   * @param opacity Glow opacity
   * @return SkPaint configured for glow
   */
#ifdef ZENITH_USE_SKIA
  static SkPaint createGlowPaint(SkColor color, float radius, float opacity);
#endif

  /**
   * @brief Create a shadow effect
   * @param offsetY Vertical offset
   * @param blur Blur radius
   * @param opacity Shadow opacity
   * @return SkPaint configured for shadow
   */
#ifdef ZENITH_USE_SKIA
  static SkPaint createShadowPaint(float offsetY, float blur, float opacity);
  static SkPaint createGradientPaint(SkColor topColor, SkColor bottomColor,
                                     const SkRect &bounds);
#endif

private:
  SkiaTheme();
  ~SkiaTheme() = default;

  // Delete copy/move
  SkiaTheme(const SkiaTheme &) = delete;
  SkiaTheme &operator=(const SkiaTheme &) = delete;

  void updateColorsForMode();

  ThemeMode currentMode_ = ThemeMode::Dark;
  ThemeColors colors_;

  // Physics settings per component
  SpringPhysicsSettings buttonPhysics_ = SpringPhysicsSettings::snappy();
  SpringPhysicsSettings sliderPhysics_ = SpringPhysicsSettings::smooth();
  SpringPhysicsSettings knobPhysics_ = SpringPhysicsSettings::smooth();
  SpringPhysicsSettings waveformPhysics_ = SpringPhysicsSettings::precise();
  SpringPhysicsSettings timelinePhysics_ = SpringPhysicsSettings::smooth();

  DepthStyle depthStyle_ = DepthStyle::moderate();
  Typography typography_;
  InteractionColors interaction_;
  SelectionStyle selectionStyle_;
  GPUSettings gpuSettings_;
};

/**
 * @class FlatSkiaLookAndFeel
 * @brief Custom LookAndFeel to ensure JUCE components match Skia flat design
 */
class FlatSkiaLookAndFeel : public juce::LookAndFeel_V4 {
public:
  FlatSkiaLookAndFeel();
  void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                            const juce::Colour &backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;
};

} // namespace zenith
