/**
 * @file SkiaTransportControlComponent.cpp
 * @brief Beautiful GPU-accelerated transport controls with Skia
 */

#include "SkiaTransportControlComponent.h"
#include "SkiaTheme.h"

#ifdef ZENITH_USE_SKIA

#include <cmath>

namespace zenith {

SkiaTransportControlComponent::SkiaTransportControlComponent()
    : isPlaying_(false), isRecording_(false), isLooping_(false), tempo_(120.0f), timelinePosition_(0.0)
{
    setSize(300, 60);
}

void SkiaTransportControlComponent::setIsPlaying(bool playing)
{
    if (isPlaying_ != playing)
    {
        isPlaying_ = playing;
        repaint();
    }
}

void SkiaTransportControlComponent::setIsRecording(bool recording)
{
    if (isRecording_ != recording)
    {
        isRecording_ = recording;
        repaint();
    }
}

void SkiaTransportControlComponent::setTempo(float bpm)
{
    if (tempo_ != bpm)
    {
        tempo_ = bpm;
        repaint();
    }
}

void SkiaTransportControlComponent::setTimelinePosition(double seconds)
{
    if (timelinePosition_ != seconds)
    {
        timelinePosition_ = seconds;
        repaint();
    }
}

void SkiaTransportControlComponent::paint(juce::Graphics& g)
{
    // This is a JUCE Graphics fallback - we'll use raw Skia rendering
    const auto& colors = SkiaTheme::getInstance().getColors();

    // Background
    g.fillAll(juce::Colour(SkColorGetR(colors.backgroundSecondary),
                           SkColorGetG(colors.backgroundSecondary),
                           SkColorGetB(colors.backgroundSecondary)));

    // Get Skia canvas from JUCE
    // Note: This requires platform-specific code or a Skia renderer wrapper
    // For now, render with JUCE Graphics as a polished alternative

    auto bounds = getLocalBounds();

    // Draw transport buttons
    drawPlayButton(g, 10, 10, 40, 40);
    drawStopButton(g, 55, 10, 40, 40);
    drawRecordButton(g, 100, 10, 40, 40);

    // Draw tempo display
    g.setColour(juce::Colour(SkColorGetR(colors.textPrimary),
                             SkColorGetG(colors.textPrimary),
                             SkColorGetB(colors.textPrimary)));
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText(juce::String(tempo_, 1) + " BPM", 150, 10, 100, 20,
               juce::Justification::left);

    // Draw timeline position
    g.setFont(juce::Font(12.0f));
    int minutes = static_cast<int>(timelinePosition_ / 60.0);
    int seconds = static_cast<int>(timelinePosition_) % 60;
    int centiseconds = static_cast<int>((timelinePosition_ - static_cast<int>(timelinePosition_)) * 100);
    juce::String timeStr = juce::String::formatted("%d:%02d.%02d", minutes, seconds, centiseconds);
    g.drawText(timeStr, 150, 35, 100, 20, juce::Justification::left);
}

void SkiaTransportControlComponent::drawPlayButton(juce::Graphics& g, int x, int y, int width, int height)
{
    const auto& colors = SkiaTheme::getInstance().getColors();

    auto buttonBounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height));

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(buttonBounds.translated(0, 2), 6.0f);

    // Button background
    if (isPlaying_)
    {
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(SkColorGetR(colors.success),
                         SkColorGetG(colors.success),
                         SkColorGetB(colors.success)).brighter(0.2f),
            buttonBounds.getCentreX(), buttonBounds.getY(),
            juce::Colour(SkColorGetR(colors.success),
                         SkColorGetG(colors.success),
                         SkColorGetB(colors.success)),
            buttonBounds.getCentreX(), buttonBounds.getBottom(),
            false));
    }
    else
    {
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(SkColorGetR(colors.surfaceHover),
                         SkColorGetG(colors.surfaceHover),
                         SkColorGetB(colors.surfaceHover)),
            buttonBounds.getCentreX(), buttonBounds.getY(),
            juce::Colour(SkColorGetR(colors.surfaceDefault),
                         SkColorGetG(colors.surfaceDefault),
                         SkColorGetB(colors.surfaceDefault)),
            buttonBounds.getCentreX(), buttonBounds.getBottom(),
            false));
    }
    g.fillRoundedRectangle(buttonBounds, 6.0f);

    // Border
    g.setColour(juce::Colour(SkColorGetR(colors.border),
                             SkColorGetG(colors.border),
                             SkColorGetB(colors.border)));
    g.drawRoundedRectangle(buttonBounds, 6.0f, 1.0f);

    // Play triangle icon
    juce::Path playIcon;
    float iconSize = width * 0.4f;
    float iconX = x + (width - iconSize) / 2.0f + 2.0f; // Slight right offset
    float iconY = y + (height - iconSize) / 2.0f;

    playIcon.addTriangle(iconX, iconY,
                        iconX, iconY + iconSize,
                        iconX + iconSize, iconY + iconSize / 2.0f);

    g.setColour(isPlaying_ ? juce::Colours::white :
                juce::Colour(SkColorGetR(colors.textPrimary),
                             SkColorGetG(colors.textPrimary),
                             SkColorGetB(colors.textPrimary)));
    g.fillPath(playIcon);
}

void SkiaTransportControlComponent::drawStopButton(juce::Graphics& g, int x, int y, int width, int height)
{
    const auto& colors = SkiaTheme::getInstance().getColors();

    auto buttonBounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height));

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(buttonBounds.translated(0, 2), 6.0f);

    // Button background
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(SkColorGetR(colors.surfaceHover),
                     SkColorGetG(colors.surfaceHover),
                     SkColorGetB(colors.surfaceHover)),
        buttonBounds.getCentreX(), buttonBounds.getY(),
        juce::Colour(SkColorGetR(colors.surfaceDefault),
                     SkColorGetG(colors.surfaceDefault),
                     SkColorGetB(colors.surfaceDefault)),
        buttonBounds.getCentreX(), buttonBounds.getBottom(),
        false));
    g.fillRoundedRectangle(buttonBounds, 6.0f);

    // Border
    g.setColour(juce::Colour(SkColorGetR(colors.border),
                             SkColorGetG(colors.border),
                             SkColorGetB(colors.border)));
    g.drawRoundedRectangle(buttonBounds, 6.0f, 1.0f);

    // Stop square icon
    float iconSize = width * 0.35f;
    auto stopIcon = juce::Rectangle<float>(
        x + (width - iconSize) / 2.0f,
        y + (height - iconSize) / 2.0f,
        iconSize,
        iconSize
    );

    g.setColour(juce::Colour(SkColorGetR(colors.textPrimary),
                             SkColorGetG(colors.textPrimary),
                             SkColorGetB(colors.textPrimary)));
    g.fillRoundedRectangle(stopIcon, 2.0f);
}

void SkiaTransportControlComponent::drawRecordButton(juce::Graphics& g, int x, int y, int width, int height)
{
    const auto& colors = SkiaTheme::getInstance().getColors();

    auto buttonBounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height));

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(buttonBounds.translated(0, 2), 6.0f);

    // Button background
    if (isRecording_)
    {
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(SkColorGetR(colors.danger),
                         SkColorGetG(colors.danger),
                         SkColorGetB(colors.danger)).brighter(0.2f),
            buttonBounds.getCentreX(), buttonBounds.getY(),
            juce::Colour(SkColorGetR(colors.danger),
                         SkColorGetG(colors.danger),
                         SkColorGetB(colors.danger)),
            buttonBounds.getCentreX(), buttonBounds.getBottom(),
            false));
    }
    else
    {
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(SkColorGetR(colors.surfaceHover),
                         SkColorGetG(colors.surfaceHover),
                         SkColorGetB(colors.surfaceHover)),
            buttonBounds.getCentreX(), buttonBounds.getY(),
            juce::Colour(SkColorGetR(colors.surfaceDefault),
                         SkColorGetG(colors.surfaceDefault),
                         SkColorGetB(colors.surfaceDefault)),
            buttonBounds.getCentreX(), buttonBounds.getBottom(),
            false));
    }
    g.fillRoundedRectangle(buttonBounds, 6.0f);

    // Border
    g.setColour(juce::Colour(SkColorGetR(colors.border),
                             SkColorGetG(colors.border),
                             SkColorGetB(colors.border)));
    g.drawRoundedRectangle(buttonBounds, 6.0f, 1.0f);

    // Record circle icon
    float iconSize = width * 0.4f;
    auto recordIcon = juce::Point<float>(
        x + width / 2.0f,
        y + height / 2.0f
    );

    g.setColour(isRecording_ ? juce::Colours::white :
                juce::Colour(SkColorGetR(colors.danger),
                             SkColorGetG(colors.danger),
                             SkColorGetB(colors.danger)));
    g.fillEllipse(recordIcon.x - iconSize / 2.0f,
                  recordIcon.y - iconSize / 2.0f,
                  iconSize, iconSize);
}

void SkiaTransportControlComponent::resized()
{
    // Layout components if needed
}

void SkiaTransportControlComponent::mouseDown(const juce::MouseEvent& event)
{
    // Handle button clicks with hit detection
    auto pos = event.getPosition();

    // Play button
    if (pos.x >= 10 && pos.x <= 50 && pos.y >= 10 && pos.y <= 50)
    {
        setIsPlaying(!isPlaying_);
    }
    // Stop button
    else if (pos.x >= 55 && pos.x <= 95 && pos.y >= 10 && pos.y <= 50)
    {
        setIsPlaying(false);
        setTimelinePosition(0.0);
    }
    // Record button
    else if (pos.x >= 100 && pos.x <= 140 && pos.y >= 10 && pos.y <= 50)
    {
        setIsRecording(!isRecording_);
    }
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
