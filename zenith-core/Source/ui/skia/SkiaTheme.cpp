/**
 * @file SkiaTheme.cpp
 * @brief Implementation of comprehensive theme system
 * 
 * DESIGN SYSTEM: Colors unified with ZenithLookAndFeel tokens
 * - accentMain: #00d9ff (cyan) - matches ZenithLookAndFeel::Colors::accentPrimary
 * - All neutrals match Material Design elevation system
 */

#include "SkiaTheme.h"

#ifdef ZENITH_USE_SKIA
#include <include/core/SkBlurTypes.h>
#include <include/core/SkMaskFilter.h>
#include <include/core/SkPaint.h>
#include <include/core/SkShader.h>
#include <include/effects/SkGradientShader.h>

#endif

namespace zenith {

//==============================================================================
// Color definitions - UNIFIED with ZenithLookAndFeel
//==============================================================================

// Helper to create ARGB color
constexpr SkColor ARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
  return (a << 24) | (r << 16) | (g << 8) | b;
}

// DESIGN SYSTEM: Dark mode palette - UNIFIED with ZenithLookAndFeel
// All colors now match the tokens in ZenithLookAndFeel.h
static const ThemeColors DARK_COLORS = {
    // === Neutrals (Material Design Elevation) ===
    // UNIFIED: Now matches ZenithLookAndFeel::Elevation exactly
    .bg0 = ARGB(255, 18, 18, 18),    // #121212 - dp0 (was #050608)
    .bg1 = ARGB(255, 30, 30, 30),    // #1e1e1e - dp1 (was #111418)
    .bg2 = ARGB(255, 35, 35, 35),    // #232323 - dp2 (was #181C22)
    .bg3 = ARGB(255, 39, 39, 39),    // #272727 - dp4 (was #1F242C)

    // === Borders - UNIFIED ===
    .borderSubtle = ARGB(255, 58, 58, 58), // #3a3a3a - matches borderSubtle
    .borderStrong = ARGB(255, 74, 74, 74), // #4a4a4a - matches borderMedium

    // === Text (87% opacity for primary) - UNIFIED ===
    .textStrong = ARGB(255, 222, 222, 222), // #dedede - matches textPrimary
    .textMuted = ARGB(255, 153, 153, 153),  // #999999 - matches textSecondary
    .textSubtle = ARGB(255, 97, 97, 97),    // #616161 - matches textDisabled
    .textDanger = ARGB(255, 255, 82, 82),   // #ff5252 - matches danger/recordRed

    // === Accents - UNIFIED with ZenithLookAndFeel ===
    .accentMain = ARGB(255, 0, 217, 255),     // #00d9ff - UNIFIED: matches accentPrimary (was #00D4AA)
    .accentAlt = ARGB(255, 255, 140, 66),     // #ff8c42 - UNIFIED: matches accentSecondary (was #4C8DFF)
    .accentRecord = ARGB(255, 255, 82, 82),   // #ff5252 - matches recordRed
    .accentWarning = ARGB(255, 255, 193, 7),  // #ffc107 - matches warning

    // === Clip Palette - Color-coded tracks from ZenithLookAndFeel ===
    .clipDrums = ARGB(255, 204, 51, 17),     // #cc3311 - trackRedDark (low freq)
    .clipBass = ARGB(255, 222, 143, 5),      // #de8f05 - trackOrange
    .clipHarmony = ARGB(255, 68, 170, 153),  // #44aa99 - trackGreen
    .clipLeads = ARGB(255, 0, 217, 255),     // #00d9ff - trackCyan (accentPrimary)
    .clipFX = ARGB(255, 156, 39, 176),       // #9c27b0 - trackPurple

    // === Legacy Mappings - UNIFIED ===
    .background = ARGB(255, 18, 18, 18),             // dp0
    .backgroundSecondary = ARGB(255, 30, 30, 30),   // dp1
    .backgroundTertiary = ARGB(255, 35, 35, 35),    // dp2

    .surfaceDefault = ARGB(255, 39, 39, 39),        // dp4
    .surfaceHover = ARGB(255, 46, 46, 46),          // dp8
    .surfaceActive = ARGB(255, 30, 30, 30),         // dp1
    .surfaceDisabled = ARGB(255, 18, 18, 18),       // dp0

    // Primary accent - UNIFIED with accentPrimary
    .primary = ARGB(255, 0, 217, 255),              // #00d9ff
    .primaryHover = ARGB(255, 51, 224, 255),        // #33e0ff - accentPrimaryHover
    .primaryActive = ARGB(255, 0, 168, 204),        // #00a8cc - accentPrimaryPressed

    // Semantic colors - UNIFIED
    .success = ARGB(255, 76, 175, 80),              // #4caf50 - playGreen
    .successHover = ARGB(255, 102, 187, 106),       // slightly lighter

    .danger = ARGB(255, 255, 82, 82),               // #ff5252 - recordRed
    .dangerHover = ARGB(255, 255, 112, 112),

    .warning = ARGB(255, 255, 193, 7),              // #ffc107
    .warningHover = ARGB(255, 255, 213, 79),

    // Text - UNIFIED
    .textPrimary = ARGB(255, 222, 222, 222),        // #dedede
    .textSecondary = ARGB(255, 153, 153, 153),      // #999999
    .textDisabled = ARGB(255, 97, 97, 97),          // #616161
    .textOnAccent = ARGB(255, 0, 0, 0),             // #000000 - black on bright backgrounds

    // Borders - UNIFIED
    .border = ARGB(255, 58, 58, 58),                // #3a3a3a
    .borderHover = ARGB(255, 74, 74, 74),           // #4a4a4a
    .divider = ARGB(255, 58, 58, 58),               // #3a3a3a

    // Shadows/highlights
    .shadowDark = ARGB(180, 0, 0, 0),
    .shadowLight = ARGB(20, 255, 255, 255),
    .innerShadow = ARGB(100, 0, 0, 0),
    .highlight = ARGB(40, 255, 255, 255),

    // DAW-specific - UNIFIED
    .waveformAudio = ARGB(255, 0, 217, 255),        // #00d9ff - accentPrimary
    .waveformMidi = ARGB(255, 68, 170, 153),        // #44aa99 - trackGreen
    .gridLine = ARGB(255, 58, 58, 58),              // #3a3a3a - borderSubtle
    .playhead = ARGB(255, 255, 82, 82),             // #ff5252 - recordRed
    .loopRegion = ARGB(60, 0, 217, 255),            // accentPrimary with alpha
    
    // Meter colors - UNIFIED with ZenithLookAndFeel
    .meterGreen = ARGB(255, 76, 175, 80),           // #4caf50 - meterGreen (safe zone)
    .meterYellow = ARGB(255, 255, 193, 7),          // #ffc107 - meterAmber (caution)
    .meterRed = ARGB(255, 255, 82, 82),             // #ff5252 - meterRed (clipping)
};

// Light mode palette (adjusted from dark mode)
static const ThemeColors LIGHT_COLORS = {
    // Neutrals (Inverted for light mode)
    .bg0 = ARGB(255, 250, 250, 250),        // Near white
    .bg1 = ARGB(255, 255, 255, 255),        // Pure white
    .bg2 = ARGB(255, 245, 245, 245),        // Very light gray
    .bg3 = ARGB(255, 238, 238, 238),        // Light gray

    // Borders
    .borderSubtle = ARGB(255, 224, 224, 224),
    .borderStrong = ARGB(255, 189, 189, 189),

    // Text (dark on light)
    .textStrong = ARGB(255, 33, 33, 33),
    .textMuted = ARGB(255, 117, 117, 117),
    .textSubtle = ARGB(255, 158, 158, 158),
    .textDanger = ARGB(255, 211, 47, 47),

    // Accents (same vibrant colors work on light)
    .accentMain = ARGB(255, 0, 188, 212),       // Slightly darker cyan for light mode
    .accentAlt = ARGB(255, 255, 112, 67),       // Darker orange
    .accentRecord = ARGB(255, 244, 67, 54),
    .accentWarning = ARGB(255, 255, 160, 0),

    // Clip Palette
    .clipDrums = ARGB(255, 239, 83, 80),
    .clipBass = ARGB(255, 255, 167, 38),
    .clipHarmony = ARGB(255, 102, 187, 106),
    .clipLeads = ARGB(255, 66, 165, 245),
    .clipFX = ARGB(255, 171, 71, 188),

    // Legacy Mappings
    .background = ARGB(255, 250, 250, 250),
    .backgroundSecondary = ARGB(255, 255, 255, 255),
    .backgroundTertiary = ARGB(255, 245, 245, 245),

    .surfaceDefault = ARGB(255, 238, 238, 238),
    .surfaceHover = ARGB(255, 224, 224, 224),
    .surfaceActive = ARGB(255, 255, 255, 255),
    .surfaceDisabled = ARGB(255, 250, 250, 250),

    .primary = ARGB(255, 0, 188, 212),
    .primaryHover = ARGB(255, 0, 172, 193),
    .primaryActive = ARGB(255, 0, 151, 167),

    .success = ARGB(255, 67, 160, 71),
    .successHover = ARGB(255, 76, 175, 80),

    .danger = ARGB(255, 211, 47, 47),
    .dangerHover = ARGB(255, 229, 57, 53),

    .warning = ARGB(255, 255, 160, 0),
    .warningHover = ARGB(255, 255, 179, 0),

    .textPrimary = ARGB(255, 33, 33, 33),
    .textSecondary = ARGB(255, 117, 117, 117),
    .textDisabled = ARGB(255, 158, 158, 158),
    .textOnAccent = ARGB(255, 255, 255, 255),

    .border = ARGB(255, 224, 224, 224),
    .borderHover = ARGB(255, 189, 189, 189),
    .divider = ARGB(255, 224, 224, 224),

    .shadowDark = ARGB(60, 0, 0, 0),
    .shadowLight = ARGB(255, 255, 255, 255),
    .innerShadow = ARGB(30, 0, 0, 0),
    .highlight = ARGB(200, 255, 255, 255),

    .waveformAudio = ARGB(255, 0, 188, 212),
    .waveformMidi = ARGB(255, 102, 187, 106),
    .gridLine = ARGB(255, 224, 224, 224),
    .playhead = ARGB(255, 244, 67, 54),
    .loopRegion = ARGB(60, 0, 188, 212),
    .meterGreen = ARGB(255, 67, 160, 71),
    .meterYellow = ARGB(255, 255, 160, 0),
    .meterRed = ARGB(255, 211, 47, 47),
};

//==============================================================================
// SkiaTheme implementation
//==============================================================================

SkiaTheme::SkiaTheme() {
  // Initialize with dark mode by default
  updateColorsForMode();

  // DESIGN SYSTEM: Interaction colors - unified with accent
  interaction_.hoverOverlay = ARGB(13, 255, 255, 255);  // ~5% white overlay
  interaction_.activeOverlay = ARGB(33, 255, 255, 255); // ~13% white overlay
  interaction_.focusBorder = ARGB(255, 0, 217, 255);    // accentPrimary #00d9ff

  // DESIGN SYSTEM: Selection style - unified with accent
  selectionStyle_.mode = SelectionMode::BorderAndTint;
  selectionStyle_.borderColor = ARGB(255, 0, 217, 255);  // accentPrimary
  selectionStyle_.borderWidth = 2.0f;
  selectionStyle_.tintColor = ARGB(38, 0, 217, 255);     // accentPrimary with 15% opacity
  selectionStyle_.tintOpacity = 0.15f;
  selectionStyle_.cornerRadius = 6.0f;  // Match ZenithLookAndFeel::Radius::m

  // Configure GPU settings
  gpuSettings_.adaptiveFPS = true;
  gpuSettings_.prioritizeQuality = true;
  gpuSettings_.targetFPS = 60;
  gpuSettings_.waveformDetailLevel = 4;
  gpuSettings_.msaaSamples = 4;

  // Configure typography
  typography_.enableTextGlow = true;
  typography_.textGlowRadius = 4.0f;
  typography_.textGlowOpacity = 0.5f;
  typography_.enableTextShadow = true;
  typography_.textShadowOffsetY = 1.5f;
  typography_.textShadowBlur = 3.0f;
  typography_.mono = {11.0f, false};

  // Set depth style
  depthStyle_ = DepthStyle::moderate();

  // Configure spring physics per component type
  buttonPhysics_ = SpringPhysicsSettings::snappy();
  sliderPhysics_ = SpringPhysicsSettings::smooth();
  knobPhysics_ = SpringPhysicsSettings::smooth();
  waveformPhysics_ = SpringPhysicsSettings::precise();
  timelinePhysics_ = SpringPhysicsSettings::smooth();
}

SkiaTheme &SkiaTheme::getInstance() {
  static SkiaTheme instance;
  return instance;
}

void SkiaTheme::setThemeMode(ThemeMode mode) {
  if (currentMode_ != mode) {
    currentMode_ = mode;
    updateColorsForMode();
  }
}

void SkiaTheme::updateColorsForMode() {
  colors_ = (currentMode_ == ThemeMode::Dark) ? DARK_COLORS : LIGHT_COLORS;
}

//==============================================================================
// Helper methods for creating effects
//==============================================================================

#ifdef ZENITH_USE_SKIA

SkPaint SkiaTheme::createGradientPaint(SkColor topColor, SkColor bottomColor,
                                       const SkRect &bounds) {
  SkPaint paint;
  paint.setAntiAlias(true);

  SkPoint points[2] = {SkPoint::Make(bounds.centerX(), bounds.top()),
                       SkPoint::Make(bounds.centerX(), bounds.bottom())};
  SkColor colors[2] = {topColor, bottomColor};
  SkScalar positions[2] = {0.0f, 1.0f};

  sk_sp<SkShader> shader = SkGradientShader::MakeLinear(
      points, colors, positions, 2, SkTileMode::kClamp);
  paint.setShader(shader);

  return paint;
}

SkPaint SkiaTheme::createGlowPaint(SkColor color, float radius, float opacity) {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(color);

  // Extract color components and apply opacity
  uint8_t a = static_cast<uint8_t>((SkColorGetA(color) * opacity));
  uint8_t r = SkColorGetR(color);
  uint8_t g = SkColorGetG(color);
  uint8_t b = SkColorGetB(color);
  paint.setColor(ARGB(a, r, g, b));

  if (radius > 0.0f) {
    paint.setMaskFilter(
        SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, radius));
  }

  return paint;
}

SkPaint SkiaTheme::createShadowPaint(float offsetY, float blur, float opacity) {
  SkPaint paint;
  paint.setAntiAlias(true);

  uint8_t alpha = static_cast<uint8_t>(255 * opacity);
  paint.setColor(ARGB(alpha, 0, 0, 0));

  if (blur > 0.0f) {
    paint.setMaskFilter(
        SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, blur));
  }

  return paint;
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
