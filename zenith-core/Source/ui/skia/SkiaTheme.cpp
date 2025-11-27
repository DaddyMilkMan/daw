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
<<<<<<< HEAD
    #include <include/core/SkPaint.h>
    #include <include/core/SkShader.h>
    #include <include/core/SkMaskFilter.h>
    #include <include/core/SkBlurTypes.h>
    #include <include/core/SkRRect.h>
    #include <include/core/SkPath.h>
    #include <include/core/SkFont.h>
    #include <include/core/SkTextBlob.h>
    #include <include/effects/SkGradientShader.h>
=======
#include <include/effects/SkGradientShader.h>
#include <include/effects/SkImageFilters.h>
>>>>>>> 9f38c723266f5a7d2dfa29ac0a1897af495d401c
#endif

#include <cstring>   // For strlen, memcpy
#include <algorithm> // For std::max, std::min

namespace zenith {

SkiaTheme &SkiaTheme::getInstance() {
  static SkiaTheme instance;
  return instance;
}

<<<<<<< HEAD
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
=======
SkiaTheme::SkiaTheme() { updateColorsForMode(); }

void SkiaTheme::setThemeMode(ThemeMode mode) {
  if (currentMode_ != mode) {
    currentMode_ = mode;
>>>>>>> 9f38c723266f5a7d2dfa29ac0a1897af495d401c
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

<<<<<<< HEAD
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

void SkiaTheme::drawFaderCap(SkCanvas* canvas, const SkRect& bounds)
{
    // Chrome/silver gradient
    SkColor topColor = ARGB(255, 221, 221, 221);    // #DDDDDD
    SkColor bottomColor = ARGB(255, 136, 136, 136); // #888888
    
    SkPaint gradientPaint = createGradientPaint(topColor, bottomColor, bounds);
    gradientPaint.setAntiAlias(true);
    drawRoundedRect(canvas, bounds, 2.0f, gradientPaint, nullptr, 0.0f);
    
    // Center line for concave effect
    SkPaint linePaint;
    linePaint.setColor(ARGB(255, 100, 100, 100));
    linePaint.setStrokeWidth(1.0f);
    linePaint.setAntiAlias(true);
    canvas->drawLine(bounds.left() + 2, bounds.centerY(), bounds.right() - 2, bounds.centerY(), linePaint);
}

void SkiaTheme::drawAudioMeter(SkCanvas* canvas, const SkRect& bounds, float level)
{
    level = std::max(0.0f, std::min(1.0f, level));
    
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(ARGB(255, 17, 17, 17));
    bgPaint.setAntiAlias(true);
    canvas->drawRect(bounds, bgPaint);
    
    // Meter fill
    float fillHeight = bounds.height() * level;
    SkRect fillRect = SkRect::MakeLTRB(bounds.left(), bounds.bottom() - fillHeight, bounds.right(), bounds.bottom());
    
    // Color zones: Green 0-0.75, Yellow 0.75-0.9, Orange 0.9-0.95, Red 0.95-1.0
    SkColor meterColor;
    if (level < 0.75f)
        meterColor = ARGB(255, 0, 255, 0);
    else if (level < 0.9f)
        meterColor = ARGB(255, 255, 255, 0);
    else if (level < 0.95f)
        meterColor = ARGB(255, 255, 153, 0);
    else
        meterColor = ARGB(255, 255, 0, 0);
    
    SkPaint meterPaint;
    meterPaint.setColor(meterColor);
    meterPaint.setAntiAlias(true);
    canvas->drawRect(fillRect, meterPaint);
}

void SkiaTheme::drawWaveform(SkCanvas* canvas, const SkRect& bounds,
                            const float* samples, int numSamples, SkColor color)
{
    if (numSamples < 2 || samples == nullptr) return;
    
    SkPath waveformPath;
    float xStep = bounds.width() / (float)numSamples;
    float halfHeight = bounds.height() * 0.5f;
    float centerY = bounds.centerY();
    
    waveformPath.moveTo(bounds.left(), centerY);
    
    // Top half
    for (int i = 0; i < numSamples; ++i)
    {
        float x = bounds.left() + i * xStep;
        float y = centerY - (samples[i] * halfHeight);
        waveformPath.lineTo(x, y);
    }
    
    // Bottom half (mirror)
    for (int i = numSamples - 1; i >= 0; --i)
    {
        float x = bounds.left() + i * xStep;
        float y = centerY + (samples[i] * halfHeight);
        waveformPath.lineTo(x, y);
    }
    
    waveformPath.close();
    
    SkPaint fillPaint;
    fillPaint.setColor(color);
    fillPaint.setAntiAlias(true);
    fillPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawPath(waveformPath, fillPaint);
}

void SkiaTheme::drawLCDText(SkCanvas* canvas, const SkPoint& position,
                           const char* text, SkColor color, float size)
{
    SkFont font;
    font.setSize(size);
    font.setEdging(SkFont::Edging::kAntiAlias);
    
    // Glow effect
    SkPaint glowPaint = createGlowPaint(color, 4.0f, 0.6f);
    glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(SkBlurStyle::kOuter_SkBlurStyle, 3.0f));
    
    SkTextBlobBuilder glowBuilder;
    const auto& glowBuffer = glowBuilder.allocRun(font, strlen(text), position.fX, position.fY);
    memcpy(glowBuffer.glyphs, text, strlen(text));
    canvas->drawTextBlob(glowBuilder.make(), 0, 0, glowPaint);
    
    // Main text
    SkPaint textPaint;
    textPaint.setColor(color);
    textPaint.setAntiAlias(true);
    
    SkTextBlobBuilder builder;
    const auto& buffer = builder.allocRun(font, strlen(text), position.fX, position.fY);
    memcpy(buffer.glyphs, text, strlen(text));
    canvas->drawTextBlob(builder.make(), 0, 0, textPaint);
}

void SkiaTheme::drawPlayhead(SkCanvas* canvas, float x, const SkRect& rulerBounds,
                            const SkRect& arrangementBounds)
{
    SkPaint playheadPaint;
    playheadPaint.setColor(ARGB(255, 255, 255, 255));  // White
    playheadPaint.setStrokeWidth(2.0f);
    playheadPaint.setAntiAlias(true);
    
    // Vertical line
    canvas->drawLine(x, rulerBounds.top(), x, arrangementBounds.bottom(), playheadPaint);
    
    // Triangle cap at top
    SkPath topTriangle;
    float triSize = 8.0f;
    topTriangle.moveTo(x, rulerBounds.bottom());
    topTriangle.lineTo(x - triSize, rulerBounds.bottom() - triSize);
    topTriangle.lineTo(x + triSize, rulerBounds.bottom() - triSize);
    topTriangle.close();
    
    SkPaint trianglePaint;
    trianglePaint.setColor(ARGB(255, 255, 255, 255));
    trianglePaint.setAntiAlias(true);
    trianglePaint.setStyle(SkPaint::kFill_Style);
    canvas->drawPath(topTriangle, trianglePaint);
    
    // Triangle cap at bottom
    SkPath bottomTriangle;
    bottomTriangle.moveTo(x, arrangementBounds.bottom());
    bottomTriangle.lineTo(x - triSize, arrangementBounds.bottom() - triSize);
    bottomTriangle.lineTo(x + triSize, arrangementBounds.bottom() - triSize);
    bottomTriangle.close();
    canvas->drawPath(bottomTriangle, trianglePaint);
}

#endif // ZENITH_USE_SKIA
=======
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
>>>>>>> 9f38c723266f5a7d2dfa29ac0a1897af495d401c

} // namespace zenith
