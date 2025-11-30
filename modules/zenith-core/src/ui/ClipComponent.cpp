/**
 * @file ClipComponent.cpp
 * @brief Flat clip component with theme colors and clean typography
 *
 * Clean DAW design:
 * - Track-colored fills (muted, from theme)
 * - Typography.body for clip names
 * - Simple 1-2px selection border
 * - Rounded corners (4px, 8px grid)
 */

// POLISH: spacing normalized to 8px grid (rounded corners 4px)
// POLISH: typography now uses SkiaTheme::Typography (body)
// POLISH: flattened visuals (track colors, no gradients)

#include "../../include/ui/ClipComponent.h"
#include "../../include/ProjectState.h"

#ifdef ZENITH_USE_SKIA
#include "../ui/skia/SkiaTheme.h"
#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#endif

ClipComponent::ClipComponent(juce::ValueTree clipNode)
    : clip(clipNode)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    startTimerHz(60);  // 60 Hz for smooth animations
}

ClipComponent::~ClipComponent()
{
    stopTimer();
}

juce::String ClipComponent::getClipId() const
{
    return clip[ProjectState::PROP_ID].toString();
}

double ClipComponent::getStartBeats() const
{
    return clip[ProjectState::PROP_START_BEATS];
}

double ClipComponent::getLengthBeats() const
{
    return clip[ProjectState::PROP_LENGTH_BEATS];
}

void ClipComponent::updateBounds(double pixelsPerBeat, int yPosition, int height)
{
    int x = static_cast<int>(getStartBeats() * pixelsPerBeat);
    int width = static_cast<int>(getLengthBeats() * pixelsPerBeat);
    setBounds(x, yPosition, width, height);
}

#ifdef ZENITH_USE_SKIA
void ClipComponent::drawSkia(SkCanvas* canvas)
{
    if (!canvas) return;

    auto bounds = getLocalBounds();
    auto& theme = ::zenith::SkiaTheme::getInstance();
    auto& colors = theme.getColors();
    auto& typo = theme.getTypography();

    // POLISH: Flat track-colored fills (no gradients)
    juce::String clipType = clip[ProjectState::PROP_TYPE].toString();

    // Map clip type to theme colors
    SkColor clipColor;
    if (clipType == "midi") {
        clipColor = colors.waveformMidi;  // Green for MIDI
    } else {
        clipColor = colors.waveformAudio;  // Blue for audio
    }

    // Create muted version of color for fill (reduce alpha for subtlety)
    SkColor fillColor = SkColorSetARGB(
        180,  // Muted alpha
        SkColorGetR(clipColor),
        SkColorGetG(clipColor),
        SkColorGetB(clipColor)
    );

    // POLISH: Rounded rect at 4px (8px grid)
    SkRect clipRect = SkRect::MakeXYWH(0, 0, bounds.getWidth(), bounds.getHeight());
    SkRRect clipRRect = SkRRect::MakeRectXY(clipRect, 4.0f, 4.0f);

    // Fill background
    SkPaint fillPaint;
    fillPaint.setAntiAlias(true);
    fillPaint.setColor(fillColor);
    canvas->drawRRect(clipRRect, fillPaint);

    // POLISH: Simple 1-2px border for selection (no pulse animation)
    if (isSelected) {
        SkPaint selectionPaint;
        selectionPaint.setAntiAlias(true);
        selectionPaint.setColor(clipColor);
        selectionPaint.setStyle(SkPaint::kStroke_Style);
        selectionPaint.setStrokeWidth(2.0f);
        canvas->drawRRect(clipRRect, selectionPaint);
    } else {
        // Subtle border
        SkPaint borderPaint;
        borderPaint.setAntiAlias(true);
        borderPaint.setColor(colors.borderSubtle);
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setStrokeWidth(1.0f);
        canvas->drawRRect(clipRRect, borderPaint);
    }

    // POLISH: Clip name using Typography.body
    if (bounds.getWidth() > 20) {
        SkFont font;
        font.setSize(typo.body.size);
        if (typo.body.bold) font.setEmbolden(true);
        font.setEdging(SkFont::Edging::kAntiAlias);

        juce::String clipName = getClipId();

        // POLISH: Ensure text legibility on clip color
        // Use textStrong for good contrast on muted backgrounds
        SkPaint textPaint;
        textPaint.setAntiAlias(true);
        textPaint.setColor(colors.textStrong);

        float textX = 8.0f;  // 8px padding
        float textY = bounds.getHeight() / 2.0f + typo.body.size / 2.0f;
        canvas->drawSimpleText(clipName.toRawUTF8(), clipName.length(), SkTextEncoding::kUTF8, textX, textY, font, textPaint);
    }
}
#else
void ClipComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Determine beautiful colors based on clip type (modern DAW palette)
    juce::Colour baseColor;
    juce::String clipType = clip[ProjectState::PROP_TYPE].toString();

    if (clipType == "midi") {
        baseColor = juce::Colour(0xff34c759);  // Apple green for MIDI
    } else {
        baseColor = juce::Colour(0xff4a9eff);  // Apple blue for audio
    }

    // Apply hover and selection scaling
    auto scaledBounds = bounds;
    if (isHovered || isSelected) {
        float scale = isSelected ? 0.98f : (isHovered ? 1.02f : 1.0f);
        scale = juce::jlimit(0.95f, 1.05f, scale + hoverAnimation * 0.05f);

        float centerX = bounds.getCentreX();
        float centerY = bounds.getCentreY();
        float newWidth = bounds.getWidth() * scale;
        float newHeight = bounds.getHeight() * scale;

        scaledBounds = juce::Rectangle<float>(
            centerX - newWidth / 2.0f,
            centerY - newHeight / 2.0f,
            newWidth,
            newHeight
        );
    }

    // Draw subtle shadow for depth (modern DAW style)
    if (!isHovered) {
        g.setColour(juce::Colour(0x00000000).withAlpha(0.3f));
        g.fillRoundedRectangle(scaledBounds.translated(0.0f, 2.0f), 8.0f);
    }

    // Draw beautiful gradient background (Ableton-style: lighter at top, darker at bottom)
    juce::ColourGradient gradient(
        baseColor.brighter(0.2f), scaledBounds.getCentreX(), scaledBounds.getY(),
        baseColor.darker(0.3f), scaledBounds.getCentreX(), scaledBounds.getBottom(),
        false
    );
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(scaledBounds, 8.0f);

    // Add subtle inner highlight (top 30%) for depth
    g.setColour(juce::Colour(0xffffffff).withAlpha(0.15f));
    auto highlightBounds = scaledBounds.withHeight(scaledBounds.getHeight() * 0.3f);
    g.fillRoundedRectangle(highlightBounds, 8.0f);

    // Draw selection glow/ring with pulse animation
    if (isSelected) {
        float glowAlpha = 0.4f + 0.2f * std::sin(selectionPulse * juce::MathConstants<float>::twoPi);
        g.setColour(baseColor.brighter(0.5f).withAlpha(glowAlpha));
        g.drawRoundedRectangle(scaledBounds.expanded(2.0f), 8.0f, 3.0f);
    }

    // Draw hover glow
    if (isHovered && !isSelected) {
        g.setColour(baseColor.brighter(0.3f).withAlpha(0.3f));
        g.drawRoundedRectangle(scaledBounds.expanded(1.0f), 8.0f, 2.0f);
    }

    // Border (subtle, modern)
    g.setColour(baseColor.darker(0.2f).withAlpha(0.8f));
    g.drawRoundedRectangle(scaledBounds.reduced(0.5f), 8.0f, 1.5f);

    // Clip name with better typography
    g.setColour(juce::Colour(0xffffffff).withAlpha(0.95f));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));

    juce::String clipName = getClipId();
    auto textBounds = scaledBounds.reduced(8.0f, 4.0f);
    g.drawText(clipName, textBounds.toNearestInt(), juce::Justification::centredLeft, true);

    // Optional: Draw waveform preview hint for audio clips (simplified for now)
    if (clipType == "audio" && scaledBounds.getWidth() > 40.0f) {
        g.setColour(juce::Colour(0xffffffff).withAlpha(0.1f));
        auto waveformBounds = scaledBounds.reduced(4.0f, scaledBounds.getHeight() * 0.35f);

        // Draw simplified waveform representation
        for (int i = 0; i < 20; ++i) {
            float x = waveformBounds.getX() + (waveformBounds.getWidth() / 20.0f) * i;
            float height = std::sin(i * 0.5f) * waveformBounds.getHeight() * 0.4f;
            g.drawLine(x, waveformBounds.getCentreY() - height,
                      x, waveformBounds.getCentreY() + height, 1.0f);
        }
    }
}
#endif

void ClipComponent::mouseEnter(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    isHovered = true;
    repaint();
}

void ClipComponent::mouseExit(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    isHovered = false;
    repaint();
}

void ClipComponent::mouseDown(const juce::MouseEvent& event)
{
    dragStartPos = event.getPosition();
    dragStartBeats = getStartBeats();

    // Toggle selection on click (Ctrl/Cmd for multi-select)
    if (!event.mods.isCommandDown()) {
        isSelected = !isSelected;
    }

    repaint();
}

void ClipComponent::mouseDrag(const juce::MouseEvent& event)
{
    // Simple drag visualization (actual state changes would go through ProjectState)
    auto delta = event.getPosition() - dragStartPos;
    setTopLeftPosition(getX() + delta.x, getY());
}

void ClipComponent::timerCallback()
{
    // Smooth animation updates
    const float animationSpeed = 0.1f;

    // Hover animation (smooth ease in/out)
    float targetHover = isHovered ? 1.0f : 0.0f;
    hoverAnimation += (targetHover - hoverAnimation) * animationSpeed;

    // Selection pulse animation
    if (isSelected) {
        selectionPulse += 0.02f;
        if (selectionPulse > 1.0f) {
            selectionPulse -= 1.0f;
        }
    }

    // Repaint only if animation is active
    if (std::abs(hoverAnimation - targetHover) > 0.01f || isSelected) {
        repaint();
    }
}

