/**
 * @file SkiaTheme.cpp
 * @brief Implementation of comprehensive theme system
 */

#include "SkiaTheme.h"

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkPaint.h>
    #include <include/core/SkShader.h>
    #include <include/core/SkGradientShader.h>
    #include <include/core/SkMaskFilter.h>
    #include <include/effects/SkGradientShader.h>
#endif

namespace zenith {

//==============================================================================
// Color definitions
//==============================================================================

// Helper to create ARGB color
constexpr SkColor ARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

// Dark mode palette (modern, professional)
static const ThemeColors DARK_COLORS = {
    // Background colors
    .background = ARGB(255, 18, 18, 18),          // #121212 - Very dark grey
    .backgroundSecondary = ARGB(255, 24, 24, 24), // #181818 - Slightly lighter
    .backgroundTertiary = ARGB(255, 32, 32, 32),  // #202020 - Raised surfaces

    // Surface colors
    .surfaceDefault = ARGB(255, 42, 42, 42),      // #2A2A2A - Default control
    .surfaceHover = ARGB(255, 52, 52, 52),        // #343434 - Hovered
    .surfaceActive = ARGB(255, 62, 62, 62),       // #3E3E3E - Pressed
    .surfaceDisabled = ARGB(255, 28, 28, 28),     // #1C1C1C - Disabled

    // Accent colors - Apple-inspired
    .primary = ARGB(255, 10, 132, 255),           // #0A84FF - Blue
    .primaryHover = ARGB(255, 40, 152, 255),      // Lighter blue
    .primaryActive = ARGB(255, 0, 112, 235),      // Darker blue

    .success = ARGB(255, 52, 199, 89),            // #34C759 - Green
    .successHover = ARGB(255, 72, 219, 109),      // Lighter green

    .danger = ARGB(255, 255, 69, 58),             // #FF453A - Red
    .dangerHover = ARGB(255, 255, 99, 88),        // Lighter red

    .warning = ARGB(255, 255, 159, 10),           // #FF9F0A - Orange
    .warningHover = ARGB(255, 255, 179, 50),      // Lighter orange

    // Text colors
    .textPrimary = ARGB(255, 255, 255, 255),      // White
    .textSecondary = ARGB(180, 255, 255, 255),    // 70% white
    .textDisabled = ARGB(100, 255, 255, 255),     // 40% white
    .textOnAccent = ARGB(255, 255, 255, 255),     // White on colors

    // Border/divider
    .border = ARGB(80, 255, 255, 255),            // 31% white
    .borderHover = ARGB(120, 255, 255, 255),      // 47% white
    .divider = ARGB(50, 255, 255, 255),           // 20% white

    // Depth/shadow
    .shadowDark = ARGB(180, 0, 0, 0),             // 70% black shadow
    .shadowLight = ARGB(255, 60, 60, 60),         // Light grey highlight
    .innerShadow = ARGB(100, 0, 0, 0),            // 40% black inner shadow
    .highlight = ARGB(40, 255, 255, 255),         // 16% white highlight

    // DAW-specific
    .waveformAudio = ARGB(255, 100, 200, 255),    // Light blue
    .waveformMidi = ARGB(255, 126, 211, 33),      // #7ED321 - Bright green
    .gridLine = ARGB(40, 255, 255, 255),          // 16% white
    .playhead = ARGB(255, 255, 69, 58),           // Red
    .loopRegion = ARGB(60, 10, 132, 255),         // 24% blue
    .meterGreen = ARGB(255, 52, 199, 89),         // Green
    .meterYellow = ARGB(255, 255, 204, 0),        // Yellow
    .meterRed = ARGB(255, 255, 69, 58),           // Red
};

// Light mode palette (clean, Apple-inspired)
static const ThemeColors LIGHT_COLORS = {
    // Background colors
    .background = ARGB(255, 248, 248, 248),       // #F8F8F8 - Very light grey
    .backgroundSecondary = ARGB(255, 242, 242, 242), // #F2F2F2
    .backgroundTertiary = ARGB(255, 235, 235, 235),  // #EBEBEB

    // Surface colors
    .surfaceDefault = ARGB(255, 255, 255, 255),   // White
    .surfaceHover = ARGB(255, 245, 245, 245),     // Light grey
    .surfaceActive = ARGB(255, 230, 230, 230),    // Medium grey
    .surfaceDisabled = ARGB(255, 250, 250, 250),  // Off-white

    // Accent colors
    .primary = ARGB(255, 0, 122, 255),            // #007AFF - iOS blue
    .primaryHover = ARGB(255, 30, 142, 255),      // Lighter
    .primaryActive = ARGB(255, 0, 102, 235),      // Darker

    .success = ARGB(255, 40, 205, 65),            // #28CD41 - iOS green
    .successHover = ARGB(255, 60, 225, 85),

    .danger = ARGB(255, 255, 59, 48),             // #FF3B30 - iOS red
    .dangerHover = ARGB(255, 255, 89, 78),

    .warning = ARGB(255, 255, 149, 0),            // #FF9500 - iOS orange
    .warningHover = ARGB(255, 255, 169, 40),

    // Text colors
    .textPrimary = ARGB(255, 0, 0, 0),            // Black
    .textSecondary = ARGB(180, 0, 0, 0),          // 70% black
    .textDisabled = ARGB(100, 0, 0, 0),           // 40% black
    .textOnAccent = ARGB(255, 255, 255, 255),     // White

    // Border/divider
    .border = ARGB(80, 0, 0, 0),                  // 31% black
    .borderHover = ARGB(120, 0, 0, 0),            // 47% black
    .divider = ARGB(50, 0, 0, 0),                 // 20% black

    // Depth/shadow
    .shadowDark = ARGB(100, 0, 0, 0),             // 40% black shadow
    .shadowLight = ARGB(255, 255, 255, 255),      // White highlight
    .innerShadow = ARGB(50, 0, 0, 0),             // 20% black
    .highlight = ARGB(180, 255, 255, 255),        // 70% white highlight

    // DAW-specific
    .waveformAudio = ARGB(255, 0, 122, 255),      // Blue
    .waveformMidi = ARGB(255, 40, 205, 65),       // Green
    .gridLine = ARGB(30, 0, 0, 0),                // 12% black
    .playhead = ARGB(255, 255, 59, 48),           // Red
    .loopRegion = ARGB(40, 0, 122, 255),          // 16% blue
    .meterGreen = ARGB(255, 40, 205, 65),
    .meterYellow = ARGB(255, 255, 204, 0),
    .meterRed = ARGB(255, 255, 59, 48),
};

//==============================================================================
// SkiaTheme implementation
//==============================================================================

SkiaTheme::SkiaTheme()
{
    // Initialize with dark mode by default
    updateColorsForMode();

    // Configure GPU settings with user preferences
    gpuSettings_.adaptiveFPS = true;           // User requested adaptive FPS
    gpuSettings_.prioritizeQuality = true;     // User wants visual quality prioritized
    gpuSettings_.targetFPS = 60;               // Start at 60, will adapt
    gpuSettings_.waveformDetailLevel = 4;      // High detail for producers
    gpuSettings_.msaaSamples = 4;              // Good antialiasing

    // Configure typography for flashy effects
    typography_.enableTextGlow = true;         // User wants flashy text
    typography_.textGlowRadius = 4.0f;         // Moderate glow
    typography_.textGlowOpacity = 0.5f;        // Visible but not overpowering
    typography_.enableTextShadow = true;
    typography_.textShadowOffsetY = 1.5f;
    typography_.textShadowBlur = 3.0f;

    // Set depth style for full 3D (user requested)
    depthStyle_ = DepthStyle::moderate();      // Balance between subtle and dramatic

    // Configure spring physics per component type (user wants different settings per component)
    buttonPhysics_ = SpringPhysicsSettings::snappy();      // Buttons: fast and responsive
    sliderPhysics_ = SpringPhysicsSettings::smooth();      // Sliders: smooth tracking
    knobPhysics_ = SpringPhysicsSettings::smooth();        // Knobs: smooth rotation
    waveformPhysics_ = SpringPhysicsSettings::precise();   // Waveforms: ultra-responsive at 120Hz
    timelinePhysics_ = SpringPhysicsSettings::smooth();    // Timeline: smooth spring (user specified)
}

SkiaTheme& SkiaTheme::getInstance()
{
    static SkiaTheme instance;
    return instance;
}

void SkiaTheme::setThemeMode(ThemeMode mode)
{
    if (currentMode_ != mode)
    {
        currentMode_ = mode;
        updateColorsForMode();
    }
}

void SkiaTheme::updateColorsForMode()
{
    colors_ = (currentMode_ == ThemeMode::Dark) ? DARK_COLORS : LIGHT_COLORS;
}

//==============================================================================
// Helper methods for creating effects
//==============================================================================

#ifdef ZENITH_USE_SKIA

SkPaint SkiaTheme::createGradientPaint(SkColor topColor, SkColor bottomColor, const SkRect& bounds)
{
    SkPaint paint;
    paint.setAntiAlias(true);

    SkPoint points[2] = {
        SkPoint::Make(bounds.centerX(), bounds.top()),
        SkPoint::Make(bounds.centerX(), bounds.bottom())
    };
    SkColor colors[2] = {topColor, bottomColor};
    SkScalar positions[2] = {0.0f, 1.0f};

    sk_sp<SkShader> shader = SkGradientShader::MakeLinear(
        points, colors, positions, 2, SkTileMode::kClamp
    );
    paint.setShader(shader);

    return paint;
}

SkPaint SkiaTheme::createGlowPaint(SkColor color, float radius, float opacity)
{
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(color);

    // Extract color components and apply opacity
    uint8_t a = static_cast<uint8_t>((SkColorGetA(color) * opacity));
    uint8_t r = SkColorGetR(color);
    uint8_t g = SkColorGetG(color);
    uint8_t b = SkColorGetB(color);
    paint.setColor(ARGB(a, r, g, b));

    if (radius > 0.0f)
    {
        paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, radius));
    }

    return paint;
}

SkPaint SkiaTheme::createShadowPaint(float offsetY, float blur, float opacity)
{
    SkPaint paint;
    paint.setAntiAlias(true);

    uint8_t alpha = static_cast<uint8_t>(255 * opacity);
    paint.setColor(ARGB(alpha, 0, 0, 0));

    if (blur > 0.0f)
    {
        paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, blur));
    }

    return paint;
}

#endif // ZENITH_USE_SKIA

} // namespace zenith

