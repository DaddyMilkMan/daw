/**
 * @file SkiaTheme.cpp
 * @brief PROFESSIONAL DAW THEME - Ableton/Logic/FL Studio Quality
 *
 * Design Philosophy:
 * - DARK: True blacks with subtle gradients (not flat grey)
 * - DEPTH: Real shadows, glows, and dimensionality
 * - PREMIUM: Glass morphism, metal textures, professional materials
 * - HIERARCHY: Clear visual weight and importance
 * - FOCUSED: UI fades when not in use, highlights what's active
 */

#include "SkiaTheme.h"

#ifdef ZENITH_USE_SKIA
#include <include/effects/SkGradientShader.h>
#include <include/effects/SkImageFilters.h>
#endif

namespace zenith {

SkiaTheme &SkiaTheme::getInstance() {
  static SkiaTheme instance;
  return instance;
}

SkiaTheme::SkiaTheme() { updateColorsForMode(); }

void SkiaTheme::setThemeMode(ThemeMode mode) {
  if (currentMode_ != mode) {
    currentMode_ = mode;
    updateColorsForMode();
  }
}

void SkiaTheme::updateColorsForMode() {
  if (currentMode_ == ThemeMode::Dark) {
    // ========================================================================
    // PROFESSIONAL DARK THEME - Ableton Live / Logic Pro Inspired
    // ========================================================================

    // TRUE BLACKS - Not grey! Deep, rich blacks with subtle variations
    colors_.bg0 = 0xFF000000; // Pure black app background
    colors_.bg1 = 0xFF0A0A0A; // Almost black - main panels
    colors_.bg2 = 0xFF151515; // Dark grey - elevated surfaces
    colors_.bg3 = 0xFF1F1F1F; // Medium dark - controls, inputs

    // BORDERS - Subtle, not distracting
    colors_.borderSubtle = 0xFF252525; // Barely visible dividers
    colors_.borderStrong = 0xFF353535; // Clear but not harsh

    // TEXT - High contrast for readability
    colors_.textStrong = 0xFFFFFFFF; // Pure white - headers, important
    colors_.textMuted = 0xFFB8B8B8;  // Light grey - body text
    colors_.textSubtle = 0xFF707070; // Mid grey - disabled, hints
    colors_.textDanger = 0xFFFF4444; // Bright red - errors

    // PROFESSIONAL ACCENTS - Vibrant but sophisticated
    colors_.accentMain = 0xFF00E5FF;    // Electric cyan (not teal!)
    colors_.accentAlt = 0xFF0A84FF;     // iOS blue - selection
    colors_.accentRecord = 0xFFFF3B30;  // Danger red - recording
    colors_.accentWarning = 0xFFFFCC00; // Golden yellow - warnings

    // CLIP COLORS - Professional, saturated, distinguishable
    colors_.clipDrums = 0xFFFF6B4A;   // Bright coral red
    colors_.clipBass = 0xFFFFBF00;    // Golden yellow
    colors_.clipHarmony = 0xFF4CD964; // Vibrant green
    colors_.clipLeads = 0xFF0A84FF;   // Electric blue
    colors_.clipFX = 0xFFBF5AF2;      // Purple

    // SURFACE STATES - Interactive feedback
    colors_.surfaceDefault = colors_.bg2;
    colors_.surfaceHover = 0xFF1C1C1C;    // Subtle lightening
    colors_.surfaceActive = 0xFF252525;   // More pronounced
    colors_.surfaceDisabled = 0xFF121212; // Dimmed

    // PRIMARY ACTION - Main CTA color with states
    colors_.primary = colors_.accentMain;
    colors_.primaryHover = 0xFF1AEBFF;  // Lighter on hover
    colors_.primaryActive = 0xFF00D4E6; // Darker when pressed

    // SUCCESS - Confirmation, enabled states
    colors_.success = 0xFF30D158; // iOS green
    colors_.successHover = 0xFF48D96B;

    // DANGER - Destructive actions, errors
    colors_.danger = colors_.accentRecord;
    colors_.dangerHover = 0xFFFF5247;

    // WARNING - Caution, important info
    colors_.warning = colors_.accentWarning;
    colors_.warningHover = 0xFFFFD633;

    // TEXT ON COLORED BACKGROUNDS
    colors_.textPrimary = colors_.textStrong;
    colors_.textSecondary = colors_.textMuted;
    colors_.textDisabled = colors_.textSubtle;
    colors_.textOnAccent = 0xFF000000; // Black text on bright accents

    // BORDERS
    colors_.border = colors_.borderSubtle;
    colors_.borderHover = colors_.borderStrong;
    colors_.divider = 0xFF1A1A1A;

    // DEPTH EFFECTS - Real shadows and highlights
    colors_.shadowDark = 0x80000000;  // 50% black shadow
    colors_.shadowLight = 0x40FFFFFF; // 25% white highlight
    colors_.innerShadow = 0x60000000; // Inner shadow for depth
    colors_.highlight = 0x30FFFFFF;   // Edge highlight (shimmer)

    // WAVEFORM & VISUALIZATION
    colors_.waveformAudio = 0xFF00E5FF; // Cyan - audio clips
    colors_.waveformMidi = 0xFF0A84FF;  // Blue - MIDI clips
    colors_.gridLine = 0xFF1F1F1F;      // Subtle grid
    colors_.playhead = 0xFFFFFFFF;      // White - playback cursor
    colors_.loopRegion = 0x40FFCC00;    // Translucent yellow

    // METERS - Professional color grading
    colors_.meterGreen = 0xFF30D158;  // Safe level
    colors_.meterYellow = 0xFFFFCC00; // Warning level
    colors_.meterRed = 0xFFFF3B30;    // Clipping

    // ======================================================================
    // INTERACTION OVERLAYS - Hover/Active/Focus effects
    // ======================================================================
    interaction_.hoverOverlay = 0x14FFFFFF;  // 8% white overlay
    interaction_.activeOverlay = 0x28FFFFFF; // 16% white overlay
    interaction_.focusBorder = colors_.accentMain;

    // ======================================================================
    // SELECTION STYLE - How selected items look
    // ======================================================================
    selectionStyle_.mode = SelectionMode::BorderAndTint;
    selectionStyle_.borderColor = colors_.accentAlt;
    selectionStyle_.borderWidth = 2.0f;
    selectionStyle_.tintColor = colors_.accentAlt;
    selectionStyle_.tintOpacity = 0.12f;
    selectionStyle_.cornerRadius = 4.0f;

    // ======================================================================
    // TYPOGRAPHY - Professional font settings
    // ======================================================================
    typography_.title = {18.0f, true};  // Panel headers
    typography_.header = {15.0f, true}; // Section headers
    typography_.body = {13.0f, false};  // Default text
    typography_.small = {11.0f, false}; // Labels, hints
    typography_.tiny = {9.0f, false};   // Timestamps
    typography_.mono = {12.0f, false};  // Timecode, values

    // Text effects for premium look
    typography_.enableTextGlow = true;
    typography_.textGlowRadius = 8.0f;
    typography_.textGlowOpacity = 0.3f;
    typography_.enableTextShadow = true;
    typography_.textShadowOffsetY = 1.0f;
    typography_.textShadowBlur = 2.0f;
    typography_.textShadowOpacity = 0.6f;

    // ======================================================================
    // DEPTH STYLE - 3D effects and shadows
    // ======================================================================
    depthStyle_.shadowBlur = 12.0f;
    depthStyle_.shadowOffsetY = 4.0f;
    depthStyle_.shadowOpacity = 0.5f;
    depthStyle_.highlightOpacity = 0.2f;
    depthStyle_.innerShadowSize = 2.0f;
    depthStyle_.useGradients = true;

    // ======================================================================
    // GPU SETTINGS - Quality over performance
    // ======================================================================
    gpuSettings_.targetFPS = 60;
    gpuSettings_.adaptiveFPS = true;
    gpuSettings_.prioritizeQuality = true;
    gpuSettings_.waveformDetailLevel = 5; // Maximum detail
    gpuSettings_.enableAntialiasing = true;
    gpuSettings_.msaaSamples = 8; // High quality AA
    gpuSettings_.enableMipmaps = true;

    // ======================================================================
    // SPRING PHYSICS - Smooth, responsive animations
    // ======================================================================
    buttonPhysics_ = {500.0f, 32.0f, 60.0f};    // Snappy buttons
    sliderPhysics_ = {420.0f, 28.0f, 60.0f};    // Smooth faders
    knobPhysics_ = {380.0f, 26.0f, 60.0f};      // Fluid knobs
    waveformPhysics_ = {550.0f, 35.0f, 120.0f}; // Precise waveforms
    timelinePhysics_ = {450.0f, 30.0f, 60.0f};  // Smooth scrubbing

  } else {
    // ======================================================================
    // LIGHT THEME (Optional - most pro DAWs are dark)
    // ======================================================================
    colors_.bg0 = 0xFFFFFFFF;
    colors_.bg1 = 0xFFF5F5F5;
    colors_.bg2 = 0xFFEEEEEE;
    colors_.bg3 = 0xFFE0E0E0;

    colors_.borderSubtle = 0xFFD0D0D0;
    colors_.borderStrong = 0xFFB0B0B0;

    colors_.textStrong = 0xFF000000;
    colors_.textMuted = 0xFF505050;
    colors_.textSubtle = 0xFF909090;
    colors_.textDanger = 0xFFFF3B30;

    colors_.accentMain = 0xFF0A84FF;
    colors_.accentAlt = 0xFF5AC8FA;
    colors_.accentRecord = 0xFFFF3B30;
    colors_.accentWarning = 0xFFFFCC00;

    // ... (rest of light theme - not commonly used in DAWs)
  }
}

#ifdef ZENITH_USE_SKIA

SkPaint SkiaTheme::createGradientPaint(SkColor topColor, SkColor bottomColor,
                                       const SkRect &bounds) {
  SkPaint paint;
  paint.setAntiAlias(true);

  SkPoint pts[2] = {{bounds.left(), bounds.top()},
                    {bounds.left(), bounds.bottom()}};
  SkColor colors[2] = {topColor, bottomColor};

  sk_sp<SkShader> shader =
      SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp);

  paint.setShader(shader);
  return paint;
}

SkPaint SkiaTheme::createGlowPaint(SkColor color, float radius, float opacity) {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(color);

  // Create glow effect using blur
  sk_sp<SkImageFilter> blur = SkImageFilters::Blur(radius, radius, nullptr);
  paint.setImageFilter(blur);

  // Adjust opacity
  uint8_t alpha = static_cast<uint8_t>(SkColorGetA(color) * opacity);
  paint.setAlpha(alpha);

  return paint;
}

SkPaint SkiaTheme::createShadowPaint(float offsetY, float blur, float opacity) {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(getInstance().getColors().shadowDark);

  // Create shadow effect
  sk_sp<SkImageFilter> shadow = SkImageFilters::DropShadow(
      0, offsetY, blur, blur, paint.getColor(), nullptr);
  paint.setImageFilter(shadow);

  // Adjust opacity
  paint.setAlpha(static_cast<uint8_t>(255 * opacity));

  return paint;
}

#endif

//==============================================================================
// FlatSkiaLookAndFeel
//==============================================================================

FlatSkiaLookAndFeel::FlatSkiaLookAndFeel() {
  // Kill the gradients by setting flat colors
  setColour(juce::TextButton::buttonColourId, juce::Colour(45, 45, 45));
  setColour(juce::TextButton::buttonOnColourId, juce::Colour(60, 60, 60));
}

void FlatSkiaLookAndFeel::drawButtonBackground(
    juce::Graphics &g, juce::Button &button,
    const juce::Colour &backgroundColour, bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown) {
  // Force flat fill
  auto bounds = button.getLocalBounds().toFloat();
  g.setColour(backgroundColour);
  g.fillRoundedRectangle(bounds, 4.0f);
}

} // namespace zenith
