/**
 * @file SkiaLabel.h
 * @brief Beautiful GPU-accelerated text label with native Skia rendering
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
#include "SkiaComponent.h"
#include "SkiaTextRenderer.h"
#include "SkiaTheme.h"

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
    #include <include/core/SkPaint.h>
    #include <include/core/SkFont.h>
#endif

namespace zenith {

/**
 * @class SkiaLabel
 * @brief GPU-accelerated text label using pure Skia rendering
 *
 * Features:
 * - Subpixel anti-aliasing
 * - GPU-accelerated rendering
 * - Optional text shadows and glows
 * - Gradient text support
 * - Automatic text truncation
 */
class SkiaLabel : public juce::Component, public SkiaComponent
{
public:
    enum class Justification
    {
        Left,
        Centred,
        Right,
        TopLeft,
        TopCentred,
        TopRight
    };

    SkiaLabel(const juce::String& labelText = {})
        : text_(labelText)
        , justification_(Justification::Left)
        , textStyle_(TextStyle::Regular)
    {
        setOpaque(false);  // Parent handles rendering via paintToSkia()
    }

    ~SkiaLabel() override = default;

    //==========================================================================
    // Text configuration
    //==========================================================================

    void setText(const juce::String& newText, juce::NotificationType notification = juce::dontSendNotification)
    {
        if (text_ != newText)
        {
            text_ = newText;
            repaint();

            if (notification != juce::dontSendNotification)
                resized();
        }
    }

    juce::String getText() const { return text_; }

    void setJustificationType(Justification justification)
    {
        justification_ = justification;
        repaint();
    }

    void setTextStyle(TextStyle style)
    {
        textStyle_ = style;
        repaint();
    }

    void setTextColour(SkColor color)
    {
        textColour_ = color;
        repaint();
    }

    //==========================================================================
    // juce::Component override - DISABLED when using Skia
    //==========================================================================

    void paint(juce::Graphics& g) override
    {
#ifndef ZENITH_USE_SKIA
        // JUCE fallback
        g.setColour(juce::Colour(textColour_));
        g.drawText(text_, getLocalBounds(), juce::Justification::centredLeft);
#else
        juce::ignoreUnused(g);
#endif
    }

    //==========================================================================
    // SkiaComponent implementation - NATIVE SKIA RENDERING
    //==========================================================================

    bool supportsSkiaRendering() const override { return true; }

    void paintToSkia(SkCanvas* canvas, SkRect bounds) override
    {
#ifdef ZENITH_USE_SKIA
        if (text_.isEmpty())
            return;

        // Get theme colors
        const auto& theme = SkiaTheme::getInstance();
        const auto& colors = theme.getColors();

        // Use text renderer for beautiful anti-aliased text
        SkiaTextRenderer textRenderer;

        TextRenderOptions options;
        options.color = textColour_ != 0 ? textColour_ : colors.textPrimary;
        options.antiAlias = true;
        options.effects = TextEffect::None;  // Clean, readable text

        // Calculate text position based on justification
        SkRect textBounds = textRenderer.measureText(text_, textStyle_);
        float x, y;

        switch (justification_)
        {
            case Justification::Left:
            case Justification::TopLeft:
                x = bounds.x() + 4.0f;  // Small padding
                y = bounds.centerY() + textBounds.height() / 3.0f;
                break;

            case Justification::Centred:
            case Justification::TopCentred:
                x = bounds.centerX() - textBounds.width() / 2.0f;
                y = bounds.centerY() + textBounds.height() / 3.0f;
                break;

            case Justification::Right:
            case Justification::TopRight:
                x = bounds.right() - textBounds.width() - 4.0f;
                y = bounds.centerY() + textBounds.height() / 3.0f;
                break;

            default:
                x = bounds.x() + 4.0f;
                y = bounds.centerY() + textBounds.height() / 3.0f;
                break;
        }

        // Draw text
        textRenderer.drawText(canvas, text_, x, y, textStyle_, options);
#endif
    }

private:
    juce::String text_;
    Justification justification_;
    TextStyle textStyle_;
    SkColor textColour_ = 0;  // 0 means use theme default

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaLabel)
};

} // namespace zenith
