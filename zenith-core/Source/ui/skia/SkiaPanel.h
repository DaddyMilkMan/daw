/**
 * @file SkiaPanel.h
 * @brief Beautiful GPU-accelerated panel container with Skia
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

#ifdef ZENITH_USE_SKIA
    #include "SkiaComponent.h"
    #include "SkiaTheme.h"
    #include <include/core/SkCanvas.h>
    #include <include/core/SkPaint.h>
    #include <include/core/SkRRect.h>
    #include <include/effects/SkGradientShader.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

/**
 * @class SkiaPanel
 * @brief Beautiful container panel with depth and shadows
 */
class SkiaPanel : public juce::Component, public SkiaComponent
{
public:
    enum class Style
    {
        Flat,           ///< Flat background, no depth
        Raised,         ///< Raised with shadow
        Inset,          ///< Inset/sunken appearance
        Card            ///< Card style with rounded corners and shadow
    };

    SkiaPanel(Style style = Style::Flat)
        : style_(style)
        , cornerRadius_(0.0f)
        , showBorder_(true)
    {
        setOpaque(false);

        // Default corner radius based on style
        if (style == Style::Card)
            cornerRadius_ = 8.0f;
    }

    void setStyle(Style style)
    {
        style_ = style;
        repaint();
    }

    void setCornerRadius(float radius)
    {
        cornerRadius_ = radius;
        repaint();
    }

    void setShowBorder(bool show)
    {
        showBorder_ = show;
        repaint();
    }

    //==========================================================================
    // Component overrides
    //==========================================================================

    void paint(juce::Graphics& g) override { juce::ignoreUnused(g); }

    //==========================================================================
    // SkiaComponent implementation
    //==========================================================================

    bool supportsSkiaRendering() const override { return true; }

    void paintToSkia(SkCanvas* canvas, SkRect bounds) override
    {
        const auto& theme = SkiaTheme::getInstance();
        const auto& colors = theme.getColors();

        // Shadow (for raised and card styles)
        if (style_ == Style::Raised || style_ == Style::Card)
        {
            SkPaint shadowPaint;
            shadowPaint.setAntiAlias(true);
            shadowPaint.setColor(SkColorSetARGB(30, 0, 0, 0));

            SkRect shadowBounds = bounds.makeOffset(0, 2);
            if (cornerRadius_ > 0)
            {
                SkRRect shadowRRect = SkRRect::MakeRectXY(shadowBounds, cornerRadius_, cornerRadius_);
                canvas->drawRRect(shadowRRect, shadowPaint);
            }
            else
            {
                canvas->drawRect(shadowBounds, shadowPaint);
            }
        }

        // Main background
        SkPaint bgPaint;
        bgPaint.setAntiAlias(true);

        SkColor bgColor = style_ == Style::Inset ? colors.backgroundTertiary : colors.backgroundSecondary;

        // Gradient for raised style
        if (style_ == Style::Raised && cornerRadius_ == 0)
        {
            SkColor gradColors[2] = {
                SkColorSetARGB(255,
                    std::min<uint8_t>(255, SkColorGetR(bgColor) + 10),
                    std::min<uint8_t>(255, SkColorGetG(bgColor) + 10),
                    std::min<uint8_t>(255, SkColorGetB(bgColor) + 10)
                ),
                bgColor
            };
            SkPoint gradPoints[2] = {{bounds.x(), bounds.y()}, {bounds.x(), bounds.bottom()}};
            bgPaint.setShader(SkGradientShader::MakeLinear(gradPoints, gradColors, nullptr, 2, SkTileMode::kClamp));
        }
        else
        {
            bgPaint.setColor(bgColor);
        }

        // Draw background
        if (cornerRadius_ > 0)
        {
            SkRRect bgRRect = SkRRect::MakeRectXY(bounds, cornerRadius_, cornerRadius_);
            canvas->drawRRect(bgRRect, bgPaint);
        }
        else
        {
            canvas->drawRect(bounds, bgPaint);
        }

        // Border
        if (showBorder_)
        {
            SkPaint borderPaint;
            borderPaint.setAntiAlias(true);
            borderPaint.setStyle(SkPaint::kStroke_Style);
            borderPaint.setStrokeWidth(1.0f);
            borderPaint.setColor(colors.border);

            if (cornerRadius_ > 0)
            {
                SkRRect borderRRect = SkRRect::MakeRectXY(bounds, cornerRadius_, cornerRadius_);
                canvas->drawRRect(borderRRect, borderPaint);
            }
            else
            {
                canvas->drawRect(bounds, borderPaint);
            }
        }

        // Inner highlight (for raised style)
        if (style_ == Style::Raised)
        {
            SkPaint highlightPaint;
            highlightPaint.setAntiAlias(true);
            highlightPaint.setStyle(SkPaint::kStroke_Style);
            highlightPaint.setStrokeWidth(1.0f);
            highlightPaint.setColor(SkColorSetARGB(20, 255, 255, 255));

            SkRect highlightBounds = bounds.makeInset(1, 1);
            highlightBounds.fBottom = highlightBounds.fTop + 1;

            if (cornerRadius_ > 0)
            {
                SkRRect highlightRRect = SkRRect::MakeRectXY(highlightBounds, cornerRadius_ - 1, cornerRadius_ - 1);
                canvas->drawRRect(highlightRRect, highlightPaint);
            }
            else
            {
                canvas->drawLine(highlightBounds.left(), highlightBounds.top(),
                               highlightBounds.right(), highlightBounds.top(), highlightPaint);
            }
        }
    }

private:
    Style style_;
    float cornerRadius_;
    bool showBorder_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaPanel)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
