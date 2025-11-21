/**
 * @file SkiaTextRenderer.h
 * @brief Flashy text rendering system with GPU-accelerated effects
 *
 * Features:
 * - GPU-accelerated text rendering with Skia
 * - Glow effects (outer glow, inner glow)
 * - Drop shadows with blur
 * - Gradient text fills
 * - Outline/stroke text
 * - Multiple text styles and weights
 * - Automatic font fallbacks
 * - Sub-pixel anti-aliasing
 */

#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
    #include <include/core/SkFont.h>
    #include <include/core/SkTypeface.h>
    #include <include/core/SkTextBlob.h>
#endif

namespace zenith {

/**
 * @enum TextStyle
 * @brief Predefined text styles for DAW UI
 */
enum class TextStyle
{
    Regular,        ///< Standard body text
    Bold,           ///< Bold emphasis
    Light,          ///< Light weight
    Heading,        ///< Large heading
    Small,          ///< Small labels
    Monospace,      ///< Code/numbers
    Display         ///< Extra large display text
};

/**
 * @enum TextEffect
 * @brief Visual effects that can be applied to text
 */
enum class TextEffect
{
    None           = 0,
    Glow           = 1 << 0,  ///< Outer glow
    Shadow         = 1 << 1,  ///< Drop shadow
    Gradient       = 1 << 2,  ///< Gradient fill
    Outline        = 1 << 3,  ///< Stroke outline
    InnerGlow      = 1 << 4,  ///< Inner glow
    AllFlashy      = Glow | Shadow | Gradient  ///< Combine multiple effects (flashy!)
};

// Allow bitwise operations on TextEffect
inline TextEffect operator|(TextEffect a, TextEffect b)
{
    return static_cast<TextEffect>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(TextEffect a, TextEffect b)
{
    return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

#ifdef ZENITH_USE_SKIA

/**
 * @struct TextRenderOptions
 * @brief Configuration for text rendering
 */
struct TextRenderOptions
{
    SkColor color = SK_ColorWHITE;              ///< Primary text color
    SkColor glowColor = SK_ColorWHITE;          ///< Glow effect color
    SkColor shadowColor = SK_ColorBLACK;        ///< Shadow color
    SkColor gradientTop = SK_ColorWHITE;        ///< Gradient top color
    SkColor gradientBottom = SK_ColorWHITE;     ///< Gradient bottom color
    SkColor outlineColor = SK_ColorBLACK;       ///< Outline stroke color

    float glowRadius = 4.0f;                    ///< Glow blur radius
    float glowOpacity = 0.6f;                   ///< Glow opacity (0-1)
    float shadowOffsetX = 0.0f;                 ///< Shadow X offset
    float shadowOffsetY = 2.0f;                 ///< Shadow Y offset
    float shadowBlur = 3.0f;                    ///< Shadow blur radius
    float shadowOpacity = 0.5f;                 ///< Shadow opacity (0-1)
    float outlineWidth = 1.5f;                  ///< Outline stroke width

    TextEffect effects = TextEffect::None;      ///< Active effects
    bool antiAlias = true;                      ///< Enable anti-aliasing
    bool subpixelAA = true;                     ///< Sub-pixel anti-aliasing (LCD)

    // Factory methods for common presets
    static TextRenderOptions standard()
    {
        TextRenderOptions opts;
        opts.effects = TextEffect::None;
        return opts;
    }

    static TextRenderOptions flashy()
    {
        TextRenderOptions opts;
        opts.effects = TextEffect::AllFlashy;
        opts.glowRadius = 5.0f;
        opts.glowOpacity = 0.7f;
        opts.shadowOffsetY = 2.0f;
        opts.shadowBlur = 4.0f;
        return opts;
    }

    static TextRenderOptions glowing()
    {
        TextRenderOptions opts;
        opts.effects = TextEffect::Glow;
        opts.glowRadius = 6.0f;
        opts.glowOpacity = 0.8f;
        return opts;
    }

    static TextRenderOptions outlined()
    {
        TextRenderOptions opts;
        opts.effects = TextEffect::Outline | TextEffect::Shadow;
        opts.outlineWidth = 2.0f;
        return opts;
    }
};

/**
 * @class SkiaTextRenderer
 * @brief GPU-accelerated text rendering with effects
 *
 * Provides high-quality text rendering with visual effects:
 * - Multiple layers: shadow → glow → fill/gradient → outline
 * - Font management and caching
 * - Automatic layout and alignment
 * - GPU-optimized rendering path
 *
 * Usage:
 * @code
 * SkiaTextRenderer textRenderer;
 * textRenderer.loadFonts();
 *
 * auto opts = TextRenderOptions::flashy();
 * opts.color = SK_ColorWHITE;
 * opts.glowColor = SK_ColorBLUE;
 *
 * textRenderer.drawText(canvas, "Zenith DAW", 100, 100,
 *                      TextStyle::Heading, opts);
 * @endcode
 */
class SkiaTextRenderer
{
public:
    SkiaTextRenderer();
    ~SkiaTextRenderer();

    /**
     * @brief Load fonts for rendering
     * @return true if fonts loaded successfully
     *
     * Loads system fonts and creates typeface cache.
     * Call once during initialization.
     */
    bool loadFonts();

    /**
     * @brief Draw text with effects
     * @param canvas Skia canvas to draw on
     * @param text Text string to render
     * @param x X position (baseline)
     * @param y Y position (baseline)
     * @param style Text style
     * @param options Rendering options with effects
     */
    void drawText(SkCanvas* canvas,
                  const juce::String& text,
                  float x, float y,
                  TextStyle style = TextStyle::Regular,
                  const TextRenderOptions& options = TextRenderOptions::standard());

    /**
     * @brief Draw text centered in a rectangle
     * @param canvas Skia canvas
     * @param text Text string
     * @param bounds Rectangle to center in
     * @param style Text style
     * @param options Rendering options
     */
    void drawTextCentered(SkCanvas* canvas,
                         const juce::String& text,
                         const SkRect& bounds,
                         TextStyle style = TextStyle::Regular,
                         const TextRenderOptions& options = TextRenderOptions::standard());

    /**
     * @brief Measure text dimensions
     * @param text Text to measure
     * @param style Text style
     * @return Bounding rectangle
     */
    SkRect measureText(const juce::String& text, TextStyle style = TextStyle::Regular);

    /**
     * @brief Get font size for a text style
     * @param style Text style
     * @return Font size in pixels
     */
    float getFontSize(TextStyle style) const;

    /**
     * @brief Set base font size
     * @param size Base size (Regular style uses this)
     */
    void setBaseFontSize(float size);

private:
    // Font management
    sk_sp<SkTypeface> regularTypeface_;
    sk_sp<SkTypeface> boldTypeface_;
    sk_sp<SkTypeface> lightTypeface_;
    sk_sp<SkTypeface> monoTypeface_;

    float baseFontSize_ = 14.0f;

    // Helper methods
    SkFont getFontForStyle(TextStyle style);
    void drawTextWithEffects(SkCanvas* canvas,
                            const char* text,
                            size_t length,
                            float x, float y,
                            const SkFont& font,
                            const TextRenderOptions& options);

    void drawTextGlow(SkCanvas* canvas,
                     const char* text, size_t length,
                     float x, float y,
                     const SkFont& font,
                     const TextRenderOptions& options);

    void drawTextShadow(SkCanvas* canvas,
                       const char* text, size_t length,
                       float x, float y,
                       const SkFont& font,
                       const TextRenderOptions& options);

    void drawTextFill(SkCanvas* canvas,
                     const char* text, size_t length,
                     float x, float y,
                     const SkFont& font,
                     const TextRenderOptions& options);

    void drawTextOutline(SkCanvas* canvas,
                        const char* text, size_t length,
                        float x, float y,
                        const SkFont& font,
                        const TextRenderOptions& options);
};

#endif // ZENITH_USE_SKIA

} // namespace zenith

