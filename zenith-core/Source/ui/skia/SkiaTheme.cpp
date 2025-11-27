/**
 * @file SkiaTheme.cpp
 * @brief Implementation of comprehensive theme system
 */

#include "SkiaTheme.h"

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkPaint.h>
    #include <include/core/SkShader.h>
    #include <include/core/SkMaskFilter.h>
    #include <include/core/SkBlurTypes.h>
    #include <include/core/SkRRect.h>
    #include <include/core/SkPath.h>
    #include <include/core/SkFont.h>
    #include <include/core/SkTextBlob.h>
    #include <include/effects/SkGradientShader.h>
#endif

#include <cstring>   // For strlen, memcpy
#include <algorithm> // For std::max, std::min

namespace zenith {

//==============================================================================
// Color definitions
//==============================================================================

// Helper to create ARGB color
constexpr SkColor ARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

// Dark mode palette - Logic Pro style
static const ThemeColors DARK_COLORS = {
    // Background colors - Logic Pro gray system
    .background = ARGB(255, 31, 31, 31),          // #1F1F1F - Main window (Logic gray)
    .backgroundSecondary = ARGB(255, 43, 43, 43), // #2B2B2B - Panels
    .backgroundTertiary = ARGB(255, 38, 38, 38),  // #262626 - Track area

    // Surface colors
    .surfaceDefault = ARGB(255, 46, 46, 46),      // #2E2E2E
    .surfaceHover = ARGB(255, 62, 62, 62),        // #3E3E3E
    .surfaceActive = ARGB(255, 68, 68, 68),       // #444444 - Selected
    .surfaceDisabled = ARGB(255, 28, 28, 28),     // #1C1C1C

    // Accent colors - Logic Pro style
    .primary = ARGB(255, 0, 111, 255),            // #006FFF - Logic Blue
    .primaryHover = ARGB(255, 58, 125, 255),      // #3A7DFF - Logic Blue glow
    .primaryActive = ARGB(255, 0, 85, 204),       // #0055CC - Darker Logic Blue

    .success = ARGB(255, 0, 255, 0),              // #00FF00 - Bright green
    .successHover = ARGB(255, 50, 255, 50),       // Lighter green

    .danger = ARGB(255, 255, 0, 0),               // #FF0000 - Pure red
    .dangerHover = ARGB(255, 255, 50, 50),        // Lighter red

    .warning = ARGB(255, 255, 153, 0),            // #FF9900 - Orange
    .warningHover = ARGB(255, 255, 173, 50),      // Lighter orange

    // Text colors - Logic Pro softer white
    .textPrimary = ARGB(255, 223, 223, 223),      // #DFDFDF - Softer than pure white
    .textSecondary = ARGB(255, 154, 154, 154),    // #9A9A9A - Labels
    .textDisabled = ARGB(255, 102, 102, 102),     // #666666
    .textOnAccent = ARGB(255, 0, 0, 0),           // Black on bright colors

    // Border/divider - Logic Pro engraved style
    .border = ARGB(255, 0, 0, 0),                 // #000000 - Deep black engraved
    .borderHover = ARGB(255, 68, 68, 68),         // #444444
    .divider = ARGB(255, 17, 17, 17),             // #111111

    // Depth/shadow (40% opacity for Logic Pro)
    .shadowDark = ARGB(102, 0, 0, 0),             // 40% black shadow (Logic spec)
    .shadowLight = ARGB(255, 80, 80, 80),         // Light grey highlight
    .innerShadow = ARGB(80, 0, 0, 0),             // 31% black
    .highlight = ARGB(60, 255, 255, 255),         // Subtle white

    // DAW-specific - Logic Pro colors
    .waveformAudio = ARGB(255, 100, 150, 255),    // Blue-ish waveform
    .waveformMidi = ARGB(255, 100, 200, 100),     // Green-ish MIDI
    .gridLine = ARGB(255, 85, 85, 85),            // #555555 - Visible bar lines
    .playhead = ARGB(255, 255, 255, 255),         // #FFFFFF - White playhead
    .loopRegion = ARGB(100, 0, 255, 0),           // Green loop tint
    .meterGreen = ARGB(255, 0, 255, 0),           // #00FF00 - Pure green
    .meterYellow = ARGB(255, 255, 255, 0),        // #FFFF00 - Pure yellow
    .meterRed = ARGB(255, 255, 0, 0),             // #FF0000 - Pure red
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
        paint.setMaskFilter(SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, radius));
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
        paint.setMaskFilter(SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, blur));
    }

    return paint;
}

//==============================================================================
// Logic Pro Rendering Methods - Part 1
//==============================================================================

void SkiaTheme::drawRoundedRect(SkCanvas* canvas, const SkRect& rect, float radius,
                               const SkPaint& fillPaint, const SkPaint* strokePaint,
                               float strokeWidth)
{
    SkRRect rrect = SkRRect::MakeRectXY(rect, radius, radius);
    canvas->drawRRect(rrect, fillPaint);
    
    if (strokePaint != nullptr && strokeWidth > 0.0f)
    {
        SkPaint stroke = *strokePaint;
        stroke.setStyle(SkPaint::kStroke_Style);
        stroke.setStrokeWidth(strokeWidth);
        stroke.setAntiAlias(true);
        canvas->drawRRect(rrect, stroke);
    }
}

void SkiaTheme::drawLogicButton(SkCanvas* canvas, const SkRect& bounds, bool isPressed,
                               bool isHighlighted, SkColor accentColor)
{
    SkColor topColor, bottomColor;
    
    if (accentColor != 0)
    {
        topColor = accentColor;
        bottomColor = SkColorSetARGB(
            SkColorGetA(accentColor),
            (uint8_t)(SkColorGetR(accentColor) * 0.8f),
            (uint8_t)(SkColorGetG(accentColor) * 0.8f),
            (uint8_t)(SkColorGetB(accentColor) * 0.8f)
        );
    }
    else
    {
        if (isPressed)
        {
            topColor = ARGB(255, 50, 50, 50);
            bottomColor = ARGB(255, 40, 40, 40);
        }
        else if (isHighlighted)
        {
            topColor = ARGB(255, 70, 70, 70);
            bottomColor = ARGB(255, 60, 60, 60);
        }
        else
        {
            topColor = ARGB(255, 62, 62, 62);    // #3E3E3E Logic Pro
            bottomColor = ARGB(255, 46, 46, 46);  // #2E2E2E Logic Pro
        }
    }
    
    SkPaint gradientPaint = createGradientPaint(topColor, bottomColor, bounds);
    SkPaint strokePaint;
    strokePaint.setColor(ARGB(255, 17, 17, 17));
    strokePaint.setAntiAlias(true);
    
    drawRoundedRect(canvas, bounds, 6.0f, gradientPaint, &strokePaint, 1.0f);
}

#endif // ZENITH_USE_SKIA

} // namespace zenith

