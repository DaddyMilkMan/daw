/**
 * @file SkiaTextDisplay.h
 * @brief Text display component with native Skia rendering
 */

#pragma once

#include <JuceHeader.h>
#include "SkiaComponent.h"

#ifdef ZENITH_USE_SKIA
    #include "include/core/SkCanvas.h"
    #include "include/core/SkFont.h"
    #include "include/core/SkPaint.h"
    #include "include/core/SkTextBlob.h"
#endif

namespace zenith {

/**
 * @class SkiaTextDisplay
 * @brief Multi-line text display using native Skia rendering
 *
 * Renders text directly with SkCanvas instead of JUCE Graphics.
 * Demonstrates pure Skia rendering with anti-aliasing and custom fonts.
 */
class SkiaTextDisplay : public juce::Component, public SkiaComponent
{
public:
    SkiaTextDisplay()
    {
        setOpaque(false);  // Parent handles rendering via paintToSkia()
    }

    void setText(const juce::String& newText)
    {
        text = newText;
        repaint();
    }

    void append(const juce::String& newText)
    {
        text += newText + "\n";
        repaint();
    }

    void clear()
    {
        text.clear();
        repaint();
    }

    // juce::Component override - DISABLED when using Skia
    void paint(juce::Graphics& g) override
    {
#ifdef ZENITH_USE_SKIA
        // When Skia is active, parent handles rendering via paintToSkia()
        // Don't draw anything here - parent will call paintToSkia() directly
        juce::ignoreUnused(g);
#else
        // JUCE fallback mode
        g.fillAll(juce::Colours::red);
        g.setColour(juce::Colours::white);
        g.drawText("ERROR: Using JUCE fallback!", getLocalBounds(), juce::Justification::centred);
#endif
    }

    // SkiaComponent implementation - THIS is what gets called
    bool supportsSkiaRendering() const override { return true; }

    void paintToSkia(SkCanvas* canvas, SkRect bounds) override
    {
#ifdef ZENITH_USE_SKIA
        // Debug: Log that we're rendering
        static bool logged = false;
        if (!logged)
        {
            logged = true;
            DBG("SkiaTextDisplay::paintToSkia() called!");
            DBG("  Bounds: " << bounds.x() << "," << bounds.y() << " " << bounds.width() << "x" << bounds.height());
            DBG("  Text length: " << text.length());
        }

        // Background - DARK for contrast
        SkPaint bgPaint;
        bgPaint.setStyle(SkPaint::kFill_Style);  // EXPLICITLY set fill style
        bgPaint.setColor(0xFF0A0A0A);  // Dark gray/black
        bgPaint.setAntiAlias(true);
        canvas->drawRect(bounds, bgPaint);

        // Draw colored bars using STROKE style (outline) since fills aren't showing
        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(10.0f);  // THICK outlines

        // RED bar
        paint.setColor(0xFFFF0000);
        canvas->drawRect(SkRect::MakeXYWH(bounds.x() + 10, bounds.y() + 10, 100, 20), paint);

        // YELLOW bar
        paint.setColor(0xFFFFFF00);
        canvas->drawRect(SkRect::MakeXYWH(bounds.x() + 10, bounds.y() + 40, 100, 20), paint);

        // CYAN bar
        paint.setColor(0xFF00FFFF);
        canvas->drawRect(SkRect::MakeXYWH(bounds.x() + 10, bounds.y() + 70, 100, 20), paint);

        // MAGENTA bar
        paint.setColor(0xFFFF00FF);
        canvas->drawRect(SkRect::MakeXYWH(bounds.x() + 10, bounds.y() + 100, 100, 20), paint);

        // Border - GREEN for debugging (draw last so it's on top)
        SkPaint borderPaint;
        borderPaint.setColor(0xFF00FF00);  // GREEN border
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setStrokeWidth(3.0f);
        canvas->drawRect(bounds, borderPaint);

        // Simple text display - just show first few lines of log
        juce::String displayText = "SKIA RENDERING TEST\n\n";
        displayText += text.substring(0, 200); // First 200 chars

        // Draw text character by character using simple rects for now
        // (will fix proper text rendering later)
        // For now just show colored bars to prove Skia works
#endif
    }

private:
    juce::String text;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaTextDisplay)
};

} // namespace zenith
