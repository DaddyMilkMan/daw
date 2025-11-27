/**
 * @file TimelineRuler.cpp
 * @brief Timeline ruler implementation with flat Skia design
 */

// POLISH: spacing normalized to 8px grid (labels at Typography.small)
// POLISH: typography now uses SkiaTheme::Typography (small)
// POLISH: flattened background (bg2, no gradients)

#include "../../include/ui/TimelineRuler.h"

#ifdef ZENITH_USE_SKIA
#include "../../Source/ui/skia/SkiaTheme.h"
#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#endif

TimelineRuler::TimelineRuler()
{
    setSize(800, 30);

    // Start 60Hz animation timer for smooth hover effects
    startTimerHz(60);

    // Enable mouse events
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void TimelineRuler::setVisibleRange(double start, double length)
{
    viewStartBeat = start;
    viewLengthBeats = length;
    repaint();
}

int TimelineRuler::beatsToPixels(double beats) const
{
    return static_cast<int>((beats - viewStartBeat) * pixelsPerBeat);
}

double TimelineRuler::pixelsToBeats(int pixels) const
{
    return viewStartBeat + (pixels / pixelsPerBeat);
}

#ifdef ZENITH_USE_SKIA
void TimelineRuler::paintSkia(SkCanvas& canvas, const juce::Rectangle<int>& bounds)
{
    auto& theme = ::zenith::SkiaTheme::getInstance();
    auto& colors = theme.getColors();
    auto& typo = theme.getTypography();
    auto& interaction = theme.getInteraction();

    // POLISH: Flat background using bg2 (no gradients)
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(colors.bg2);
    canvas.drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), bgPaint);

    // Top border for separation
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setColor(colors.borderSubtle);
    canvas.drawLine(0, 0, bounds.getWidth(), 0, borderPaint);

    // Draw beat markers
    int startBeat = static_cast<int>(std::floor(viewStartBeat));
    int endBeat = static_cast<int>(std::ceil(viewStartBeat + viewLengthBeats));

    SkFont font;
    font.setSize(typo.small.size);
    if (typo.small.bold) font.setEmbolden(true);
    font.setEdging(SkFont::Edging::kAntiAlias);

    for (int beat = startBeat; beat <= endBeat; ++beat)
    {
        int x = beatsToPixels(beat);

        if (x < 0 || x > bounds.getWidth())
            continue;

        // Downbeats (measure starts) - every 4 beats
        if (beat % 4 == 0)
        {
            int measure = beat / 4 + 1;
            bool isHoveredMeasure = (measure == hoveredMeasure);

            // POLISH: Subtle highlight for hovered measure using interaction overlay
            if (isHoveredMeasure && hoverAnimation > 0.01f)
            {
                SkPaint hoverBgPaint;
                hoverBgPaint.setAntiAlias(true);
                hoverBgPaint.setColor(interaction.hoverOverlay);
                hoverBgPaint.setAlpha(static_cast<uint8_t>(SkColorGetA(interaction.hoverOverlay) * hoverAnimation));

                int nextX = beatsToPixels(beat + 4);
                if (nextX > bounds.getWidth()) nextX = bounds.getWidth();
                canvas.drawRect(SkRect::MakeXYWH(x, 0, nextX - x, bounds.getHeight()), hoverBgPaint);
            }

            // Measure line using borderStrong
            SkPaint linePaint;
            linePaint.setAntiAlias(true);
            linePaint.setColor(isHoveredMeasure ? colors.accentMain : colors.borderStrong);
            if (isHoveredMeasure)
                linePaint.setAlpha(static_cast<uint8_t>(255 * hoverAnimation * 0.6f + 255 * 0.4f));
            linePaint.setStrokeWidth(1.0f);
            canvas.drawLine(x, 0, x, bounds.getHeight(), linePaint);

            // POLISH: Measure number using Typography.small
            juce::String text = juce::String(measure);
            SkPaint textPaint;
            textPaint.setAntiAlias(true);
            textPaint.setColor(isHoveredMeasure ? colors.accentMain : colors.textMuted);
            if (isHoveredMeasure)
                textPaint.setAlpha(static_cast<uint8_t>(255 * hoverAnimation * 0.5f + 255 * 0.5f));

            canvas.drawString(text.toRawUTF8(), x + 8, bounds.getHeight() / 2 + typo.small.size / 2, font, textPaint);
        }
        else
        {
            // Minor beat ticks using borderSubtle
            SkPaint tickPaint;
            tickPaint.setAntiAlias(true);
            tickPaint.setColor(colors.borderSubtle);
            tickPaint.setStrokeWidth(0.5f);
            canvas.drawLine(x, bounds.getHeight() - 8, x, bounds.getHeight(), tickPaint);
        }
    }

    // Tooltip (preserved from original)
    if (isHovered && hoverAnimation > 0.5f)
    {
        double beatAtMouse = pixelsToBeats(mousePosition.x);
        juce::String timeText = formatTimePosition(beatAtMouse);

        SkFont tooltipFont;
        tooltipFont.setSize(typo.small.size);
        tooltipFont.setEdging(SkFont::Edging::kAntiAlias);

        SkRect textBounds;
        tooltipFont.measureText(timeText.toRawUTF8(), timeText.length(), SkTextEncoding::kUTF8, &textBounds);
        int tooltipWidth = static_cast<int>(textBounds.width()) + 16;
        int tooltipHeight = 24;

        int tooltipX = mousePosition.x - tooltipWidth / 2;
        int tooltipY = bounds.getHeight() + 4;
        tooltipX = juce::jlimit(2, bounds.getWidth() - tooltipWidth - 2, tooltipX);

        SkRect tooltipRect = SkRect::MakeXYWH(tooltipX, tooltipY, tooltipWidth, tooltipHeight);

        // POLISH: Simplified tooltip (no heavy shadows)
        SkPaint tooltipBgPaint;
        tooltipBgPaint.setAntiAlias(true);
        tooltipBgPaint.setColor(colors.bg3);
        tooltipBgPaint.setAlpha(static_cast<uint8_t>(255 * hoverAnimation));
        canvas.drawRoundRect(tooltipRect, 4.0f, 4.0f, tooltipBgPaint);

        SkPaint tooltipBorderPaint;
        tooltipBorderPaint.setAntiAlias(true);
        tooltipBorderPaint.setColor(colors.borderSubtle);
        tooltipBorderPaint.setStyle(SkPaint::kStroke_Style);
        tooltipBorderPaint.setStrokeWidth(1.0f);
        tooltipBorderPaint.setAlpha(static_cast<uint8_t>(255 * hoverAnimation));
        canvas.drawRoundRect(tooltipRect, 4.0f, 4.0f, tooltipBorderPaint);

        SkPaint tooltipTextPaint;
        tooltipTextPaint.setAntiAlias(true);
        tooltipTextPaint.setColor(colors.textStrong);
        tooltipTextPaint.setAlpha(static_cast<uint8_t>(255 * hoverAnimation));

        float textX = tooltipRect.centerX() - textBounds.width() / 2;
        float textY = tooltipRect.centerY() + typo.small.size / 2;
        canvas.drawString(timeText.toRawUTF8(), textX, textY, tooltipFont, tooltipTextPaint);
    }
}
#else
void TimelineRuler::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Draw all layers
    drawBackground(g, bounds);
    drawHoverFeedback(g, bounds);
    drawBeatMarkers(g, bounds);
    drawTooltip(g);
}
#endif

void TimelineRuler::drawBackground(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Ultra-subtle gradient - minimal and clean
    juce::ColourGradient gradient(
        juce::Colour(0xff252525), 0.0f, 0.0f,
        juce::Colour(0xff222222), 0.0f, static_cast<float>(bounds.getHeight()),
        false
    );
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    // Soft inner shadow at top for depth (instead of border)
    juce::ColourGradient shadowGradient(
        juce::Colours::black.withAlpha(0.15f), 0.0f, 0.0f,
        juce::Colours::transparentBlack, 0.0f, 3.0f,
        false
    );
    g.setGradientFill(shadowGradient);
    g.fillRect(0, 0, bounds.getWidth(), 3);

    // Soft highlight at bottom
    g.setColour(juce::Colours::white.withAlpha(0.015f));
    g.fillRect(0, bounds.getHeight() - 1, bounds.getWidth(), 1);
}

void TimelineRuler::drawBeatMarkers(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    int startBeat = static_cast<int>(std::floor(viewStartBeat));
    int endBeat = static_cast<int>(std::ceil(viewStartBeat + viewLengthBeats));

    for (int beat = startBeat; beat <= endBeat; ++beat)
    {
        int x = beatsToPixels(beat);

        if (x < 0 || x > bounds.getWidth())
            continue;

        // Downbeats (measure starts) - every 4 beats
        if (beat % 4 == 0)
        {
            int measure = beat / 4 + 1;
            bool isHoveredMeasure = (measure == hoveredMeasure);

            // Ultra-subtle highlight for hovered measure
            if (isHoveredMeasure)
            {
                g.setColour(juce::Colour(0xff0A84FF).withAlpha(0.06f));
                int nextX = beatsToPixels(beat + 4);
                if (nextX > bounds.getWidth()) nextX = bounds.getWidth();
                g.fillRect(x, 0, nextX - x, bounds.getHeight());
            }

            // Minimal beat line - thin and subtle
            g.setColour(isHoveredMeasure ?
                       juce::Colour(0xff0A84FF).withAlpha(0.4f) :
                       juce::Colours::white.withAlpha(0.12f));
            g.drawLine(static_cast<float>(x), 0.0f, static_cast<float>(x),
                      static_cast<float>(bounds.getHeight()), 1.0f);

            // Measure number - clean and minimal
            juce::String text = juce::String(measure);
            g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));

            g.setColour(isHoveredMeasure ?
                       juce::Colour(0xff0A84FF).withAlpha(0.9f) :
                       juce::Colours::white.withAlpha(0.5f));
            g.drawText(text, x + 6, 0, 40, bounds.getHeight(), juce::Justification::centredLeft);
        }
        else
        {
            // Ultra-minimal beat ticks
            g.setColour(juce::Colours::white.withAlpha(0.06f));
            g.drawLine(static_cast<float>(x),
                      bounds.getHeight() - 8.0f,
                      static_cast<float>(x),
                      static_cast<float>(bounds.getHeight()),
                      0.5f);
        }
    }
}

void TimelineRuler::drawHoverFeedback(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    if (!isHovered || hoverAnimation < 0.01f)
        return;

    // Subtle glow effect at mouse position
    float glowAlpha = hoverAnimation * 0.15f;
    g.setColour(juce::Colour(0xff0A84FF).withAlpha(glowAlpha));

    int glowRadius = 60;
    juce::Rectangle<float> glowArea(
        static_cast<float>(mousePosition.x - glowRadius / 2),
        0.0f,
        static_cast<float>(glowRadius),
        static_cast<float>(bounds.getHeight())
    );

    g.fillRect(glowArea);
}

void TimelineRuler::drawTooltip(juce::Graphics& g)
{
    if (!isHovered || hoverAnimation < 0.5f)
        return;

    // Calculate time at mouse position
    double beatAtMouse = pixelsToBeats(mousePosition.x);
    juce::String timeText = formatTimePosition(beatAtMouse);

    // Tooltip styling - Apple-inspired
    juce::Font tooltipFont(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain);
    int textWidth = tooltipFont.getStringWidth(timeText);
    int tooltipWidth = textWidth + 16;
    int tooltipHeight = 24;

    // Position tooltip above mouse, centered
    int tooltipX = mousePosition.x - tooltipWidth / 2;
    int tooltipY = getHeight() + 4; // Below the ruler

    // Keep tooltip in bounds
    tooltipX = juce::jlimit(2, getWidth() - tooltipWidth - 2, tooltipX);

    juce::Rectangle<int> tooltipBounds(tooltipX, tooltipY, tooltipWidth, tooltipHeight);

    // Tooltip background with shadow
    g.setColour(juce::Colour(0x00000000).withAlpha(0.3f * hoverAnimation));
    g.fillRoundedRectangle(tooltipBounds.translated(0, 1).toFloat(), 6.0f);

    // Tooltip background
    g.setColour(juce::Colour(0xff2C2C2E).withAlpha(hoverAnimation));
    g.fillRoundedRectangle(tooltipBounds.toFloat(), 6.0f);

    // Tooltip border
    g.setColour(juce::Colour(0xff3A3A3C).withAlpha(hoverAnimation * 0.8f));
    g.drawRoundedRectangle(tooltipBounds.toFloat(), 6.0f, 1.0f);

    // Tooltip text
    g.setColour(juce::Colours::white.withAlpha(hoverAnimation * 0.95f));
    g.setFont(tooltipFont);
    g.drawText(timeText, tooltipBounds, juce::Justification::centred);
}

juce::String TimelineRuler::formatTimePosition(double beat) const
{
    // Format: Measure.Beat (e.g., "3.2" for 3rd measure, 2nd beat)
    int measure = static_cast<int>(beat / 4.0) + 1;
    int beatInMeasure = static_cast<int>(beat) % 4 + 1;
    double fraction = beat - std::floor(beat);

    if (fraction < 0.01) // On the beat
        return juce::String::formatted("%d.%d", measure, beatInMeasure);
    else // Between beats
        return juce::String::formatted("%d.%d.%02d", measure, beatInMeasure,
                                      static_cast<int>(fraction * 100));
}

void TimelineRuler::mouseMove(const juce::MouseEvent& event)
{
    mousePosition = event.getPosition();

    // Calculate which measure is being hovered
    double beatAtMouse = pixelsToBeats(mousePosition.x);
    hoveredMeasure = static_cast<int>(beatAtMouse / 4.0) + 1;

    repaint();
}

void TimelineRuler::mouseEnter(const juce::MouseEvent& event)
{
    isHovered = true;
    mousePosition = event.getPosition();
}

void TimelineRuler::mouseExit(const juce::MouseEvent& event)
{
    isHovered = false;
    hoveredMeasure = -1;
    repaint();
}

void TimelineRuler::mouseDown(const juce::MouseEvent& event)
{
    // Click-to-seek functionality
    double beatAtClick = pixelsToBeats(event.x);

    // Snap to nearest beat
    beatAtClick = std::round(beatAtClick);

    // Call seek callback if set
    if (onSeek)
        onSeek(beatAtClick);
}

void TimelineRuler::timerCallback()
{
    // Smooth animation for hover effect (60Hz)
    const float animationSpeed = 0.2f;
    float target = isHovered ? 1.0f : 0.0f;

    hoverAnimation += (target - hoverAnimation) * animationSpeed;

    // Only repaint if animation is active
    if (std::abs(hoverAnimation - target) > 0.01f)
        repaint();
}

void TimelineRuler::resized()
{
    // Calculate pixels per beat based on width
    if (viewLengthBeats > 0)
        pixelsPerBeat = getWidth() / viewLengthBeats;
}

