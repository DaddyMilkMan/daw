/**
 * @file SkiaTextRenderer.cpp
 * @brief Implementation of flashy text rendering system
 */

#include "SkiaTextRenderer.h"
#include "SkiaTheme.h"

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkPaint.h>
    #include <include/core/SkShader.h>
    #include <include/core/SkMaskFilter.h>
    #include <include/core/SkBlurTypes.h>
    #include <include/core/SkFontMgr.h>
    #include <include/effects/SkGradientShader.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

//==============================================================================
// Constructor/Destructor
//==============================================================================

SkiaTextRenderer::SkiaTextRenderer()
{
    loadFonts();
}

SkiaTextRenderer::~SkiaTextRenderer() = default;

//==============================================================================
// Font loading
//==============================================================================

bool SkiaTextRenderer::loadFonts()
{
    // Use default typefaces (simplified for initial Skia build)
    // In newer Skia, just use empty sk_sp which will use default font
    regularTypeface_ = nullptr;
    boldTypeface_ = nullptr;
    lightTypeface_ = nullptr;
    monoTypeface_ = nullptr;

    return true;
}

//==============================================================================
// Font selection
//==============================================================================

float SkiaTextRenderer::getFontSize(TextStyle style) const
{
    const auto& typo = SkiaTheme::getInstance().getTypography();

    switch (style)
    {
        case TextStyle::Regular:    return typo.baseSize;
        case TextStyle::Bold:       return typo.baseSize;
        case TextStyle::Light:      return typo.baseSize;
        case TextStyle::Heading:    return typo.headingSize;
        case TextStyle::Small:      return typo.smallSize;
        case TextStyle::Monospace:  return typo.baseSize;
        case TextStyle::Display:    return typo.headingSize * 1.5f;
        default:                    return typo.baseSize;
    }
}

void SkiaTextRenderer::setBaseFontSize(float size)
{
    baseFontSize_ = size;
}

SkFont SkiaTextRenderer::getFontForStyle(TextStyle style)
{
    sk_sp<SkTypeface> typeface;

    switch (style)
    {
        case TextStyle::Regular:    typeface = regularTypeface_; break;
        case TextStyle::Bold:       typeface = boldTypeface_; break;
        case TextStyle::Light:      typeface = lightTypeface_; break;
        case TextStyle::Heading:    typeface = boldTypeface_; break;
        case TextStyle::Small:      typeface = regularTypeface_; break;
        case TextStyle::Monospace:  typeface = monoTypeface_; break;
        case TextStyle::Display:    typeface = boldTypeface_; break;
        default:                    typeface = regularTypeface_; break;
    }

    SkFont font(typeface, getFontSize(style));
    font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
    font.setSubpixel(true);
    font.setHinting(SkFontHinting::kSlight);

    return font;
}

//==============================================================================
// Text measurement
//==============================================================================

SkRect SkiaTextRenderer::measureText(const juce::String& text, TextStyle style)
{
    SkFont font = getFontForStyle(style);

    SkRect bounds;
    const char* textStr = text.toRawUTF8();
    font.measureText(textStr, text.getNumBytesAsUTF8(), SkTextEncoding::kUTF8, &bounds);

    return bounds;
}

//==============================================================================
// Text rendering
//==============================================================================

void SkiaTextRenderer::drawText(SkCanvas* canvas,
                                const juce::String& text,
                                float x, float y,
                                TextStyle style,
                                const TextRenderOptions& options)
{
    if (!canvas || text.isEmpty())
        return;

    SkFont font = getFontForStyle(style);
    const char* textStr = text.toRawUTF8();
    size_t length = text.getNumBytesAsUTF8();

    drawTextWithEffects(canvas, textStr, length, x, y, font, options);
}

void SkiaTextRenderer::drawTextCentered(SkCanvas* canvas,
                                       const juce::String& text,
                                       const SkRect& bounds,
                                       TextStyle style,
                                       const TextRenderOptions& options)
{
    if (!canvas || text.isEmpty())
        return;

    SkRect textBounds = measureText(text, style);

    // Center horizontally and vertically
    float x = bounds.centerX() - textBounds.width() / 2.0f;
    float y = bounds.centerY() + textBounds.height() / 2.0f - textBounds.bottom();

    drawText(canvas, text, x, y, style, options);
}

//==============================================================================
// Effect rendering (layered approach for flashy results)
//==============================================================================

void SkiaTextRenderer::drawTextWithEffects(SkCanvas* canvas,
                                          const char* text,
                                          size_t length,
                                          float x, float y,
                                          const SkFont& font,
                                          const TextRenderOptions& options)
{
    // Layer 1: Shadow (drawn first, behind everything)
    if (options.effects & TextEffect::Shadow)
    {
        drawTextShadow(canvas, text, length, x, y, font, options);
    }

    // Layer 2: Glow (behind text)
    if (options.effects & TextEffect::Glow)
    {
        drawTextGlow(canvas, text, length, x, y, font, options);
    }

    // Layer 3: Main text fill (solid or gradient)
    drawTextFill(canvas, text, length, x, y, font, options);

    // Layer 4: Outline (on top of fill)
    if (options.effects & TextEffect::Outline)
    {
        drawTextOutline(canvas, text, length, x, y, font, options);
    }
}

void SkiaTextRenderer::drawTextShadow(SkCanvas* canvas,
                                     const char* text, size_t length,
                                     float x, float y,
                                     const SkFont& font,
                                     const TextRenderOptions& options)
{
    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(options.antiAlias);

    // Apply shadow opacity
    uint8_t alpha = static_cast<uint8_t>(255 * options.shadowOpacity);
    uint8_t r = SkColorGetR(options.shadowColor);
    uint8_t g = SkColorGetG(options.shadowColor);
    uint8_t b = SkColorGetB(options.shadowColor);
    shadowPaint.setColor(SkColorSetARGB(alpha, r, g, b));

    // Apply blur
    if (options.shadowBlur > 0.0f)
    {
        shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, options.shadowBlur));
    }

    // Draw shadow offset from original position
    canvas->drawSimpleText(text, length, SkTextEncoding::kUTF8,
                          x + options.shadowOffsetX,
                          y + options.shadowOffsetY,
                          font, shadowPaint);
}

void SkiaTextRenderer::drawTextGlow(SkCanvas* canvas,
                                   const char* text, size_t length,
                                   float x, float y,
                                   const SkFont& font,
                                   const TextRenderOptions& options)
{
    // Draw multiple glow layers for more intense effect
    const int glowLayers = 3;

    for (int i = 0; i < glowLayers; ++i)
    {
        SkPaint glowPaint;
        glowPaint.setAntiAlias(options.antiAlias);

        // Each layer has decreasing opacity and increasing blur
        float layerOpacity = options.glowOpacity * (1.0f - i * 0.25f);
        float layerBlur = options.glowRadius * (1.0f + i * 0.3f);

        uint8_t alpha = static_cast<uint8_t>(255 * layerOpacity);
        uint8_t r = SkColorGetR(options.glowColor);
        uint8_t g = SkColorGetG(options.glowColor);
        uint8_t b = SkColorGetB(options.glowColor);
        glowPaint.setColor(SkColorSetARGB(alpha, r, g, b));

        if (layerBlur > 0.0f)
        {
            glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, layerBlur));
        }

        canvas->drawSimpleText(text, length, SkTextEncoding::kUTF8, x, y, font, glowPaint);
    }
}

void SkiaTextRenderer::drawTextFill(SkCanvas* canvas,
                                   const char* text, size_t length,
                                   float x, float y,
                                   const SkFont& font,
                                   const TextRenderOptions& options)
{
    SkPaint fillPaint;
    fillPaint.setAntiAlias(options.antiAlias);

    // Use gradient fill if enabled
    if (options.effects & TextEffect::Gradient)
    {
        // Measure text to create gradient bounds
        SkRect textBounds;
        font.measureText(text, length, SkTextEncoding::kUTF8, &textBounds);

        SkPoint points[2] = {
            SkPoint::Make(x, y + textBounds.top()),
            SkPoint::Make(x, y + textBounds.bottom())
        };
        SkColor colors[2] = {options.gradientTop, options.gradientBottom};
        SkScalar positions[2] = {0.0f, 1.0f};

        sk_sp<SkShader> shader = SkGradientShader::MakeLinear(
            points, colors, positions, 2, SkTileMode::kClamp
        );
        fillPaint.setShader(shader);
    }
    else
    {
        fillPaint.setColor(options.color);
    }

    canvas->drawSimpleText(text, length, SkTextEncoding::kUTF8, x, y, font, fillPaint);
}

void SkiaTextRenderer::drawTextOutline(SkCanvas* canvas,
                                      const char* text, size_t length,
                                      float x, float y,
                                      const SkFont& font,
                                      const TextRenderOptions& options)
{
    SkPaint outlinePaint;
    outlinePaint.setAntiAlias(options.antiAlias);
    outlinePaint.setStyle(SkPaint::kStroke_Style);
    outlinePaint.setStrokeWidth(options.outlineWidth);
    outlinePaint.setColor(options.outlineColor);

    canvas->drawSimpleText(text, length, SkTextEncoding::kUTF8, x, y, font, outlinePaint);
}

#endif // ZENITH_USE_SKIA

} // namespace zenith

