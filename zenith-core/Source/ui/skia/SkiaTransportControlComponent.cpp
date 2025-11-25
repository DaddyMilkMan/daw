/**
 * @file SkiaTransportControlComponent.cpp
 * @brief Beautiful GPU-accelerated transport controls with professional animations
 */

#include "SkiaTransportControlComponent.h"
#include "SkiaTheme.h"

#ifdef ZENITH_USE_SKIA

#include <cmath>

namespace zenith {

SkiaTransportControlComponent::SkiaTransportControlComponent()
    : isPlaying_(false), isRecording_(false), isLooping_(false), tempo_(120.0f), timelinePosition_(0.0),
      playButtonGlow_(0.0f), stopButtonGlow_(0.0f), recordButtonGlow_(0.0f), recordingPulse_(0.0f),
      isPlayHovered_(false), isStopHovered_(false), isRecordHovered_(false)
{
    setSize(400, 80);
    setOpaque(false);
    startTimer(16);  // 60fps animation timer
}

SkiaTransportControlComponent::~SkiaTransportControlComponent()
{
    stopTimer();
}

void SkiaTransportControlComponent::setIsPlaying(bool playing)
{
    if (isPlaying_ != playing)
    {
        isPlaying_ = playing;
        playButtonGlow_ = playing ? 1.0f : 0.0f;  // Animate to full glow when playing
        repaint();
    }
}

void SkiaTransportControlComponent::setIsRecording(bool recording)
{
    if (isRecording_ != recording)
    {
        isRecording_ = recording;
        recordButtonGlow_ = recording ? 1.0f : 0.0f;  // Animate to full glow when recording
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
    const auto& colors = SkiaTheme::getInstance().getColors();

    // Fill background
    g.fillAll(juce::Colour(SkColorGetR(colors.bg1),
                           SkColorGetG(colors.bg1),
                           SkColorGetB(colors.bg1)));

    // Draw circular buttons (40px diameter)
    const float buttonDiameter = 40.0f;
    const float spacing = 15.0f;
    const float topMargin = 20.0f;
    const float leftMargin = 20.0f;

    // Play button
    drawButtonCircular(g, leftMargin, topMargin, buttonDiameter,
                       isPlaying_, isPlayHovered_, "▶",
                       juce::Colour(SkColorGetR(colors.success),
                                   SkColorGetG(colors.success),
                                   SkColorGetB(colors.success)));

    // Stop button
    drawButtonCircular(g, leftMargin + buttonDiameter + spacing, topMargin, buttonDiameter,
                       false, isStopHovered_, "⏹",
                       juce::Colour(SkColorGetR(colors.textMuted),
                                   SkColorGetG(colors.textMuted),
                                   SkColorGetB(colors.textMuted)));

    // Record button
    drawButtonCircular(g, leftMargin + 2 * (buttonDiameter + spacing), topMargin, buttonDiameter,
                       isRecording_, isRecordHovered_, "●",
                       juce::Colour(SkColorGetR(colors.danger),
                                   SkColorGetG(colors.danger),
                                   SkColorGetB(colors.danger)));

    // Draw tempo display
    drawTempoDisplay(g, leftMargin + 3 * (buttonDiameter + spacing), topMargin,
                     getWidth() - (leftMargin + 3 * (buttonDiameter + spacing)) - 20, buttonDiameter);

    // Draw timeline display
    drawTimelineDisplay(g, leftMargin, topMargin + buttonDiameter + 15,
                        getWidth() - 2 * leftMargin, 30);
}

void SkiaTransportControlComponent::timerCallback()
{
    // Animate glow effects
    const float glowDecay = 0.15f;  // Exponential decay

    // Play button glow animation
    if (isPlaying_)
        playButtonGlow_ = std::min(1.0f, playButtonGlow_ + glowDecay);
    else
        playButtonGlow_ = std::max(0.0f, playButtonGlow_ - glowDecay);

    // Record button glow + pulse animation
    if (isRecording_)
    {
        recordButtonGlow_ = std::min(1.0f, recordButtonGlow_ + glowDecay);
        // Pulse effect: sine wave from 0 to 1 over ~2 seconds (at 60fps)
        recordingPulse_ = (std::sin(static_cast<float>(juce::Time::getMillisecondCounter()) * 0.003f) + 1.0f) / 2.0f;
    }
    else
    {
        recordButtonGlow_ = std::max(0.0f, recordButtonGlow_ - glowDecay);
        recordingPulse_ = 0.0f;
    }

    // Stop button glow (only on hover)
    if (isStopHovered_)
        stopButtonGlow_ = std::min(1.0f, stopButtonGlow_ + glowDecay);
    else
        stopButtonGlow_ = std::max(0.0f, stopButtonGlow_ - glowDecay);

    repaint();
}

void SkiaTransportControlComponent::drawButtonCircular(juce::Graphics& g, float x, float y, float diameter,
                                                       bool isActive, bool isHovered, const juce::String& icon,
                                                       const juce::Colour& activeColor)
{
    const auto& colors = SkiaTheme::getInstance().getColors();
    const auto& typo = SkiaTheme::getInstance().getTypography();

    float radius = diameter / 2.0f;
    auto center = juce::Point<float>(x + radius, y + radius);

    // Store bounds for hit testing
    juce::Rectangle<float> bounds(x, y, diameter, diameter);
    if (isActive) {
        if (activeColor == juce::Colour(SkColorGetR(colors.success),
                                       SkColorGetG(colors.success),
                                       SkColorGetB(colors.success)))
            playButtonBounds_ = bounds;
        else if (activeColor == juce::Colour(SkColorGetR(colors.danger),
                                            SkColorGetG(colors.danger),
                                            SkColorGetB(colors.danger)))
            recordButtonBounds_ = bounds;
    } else if (isHovered) {
        stopButtonBounds_ = bounds;
    }

    // Glow layer (if active or hovered)
    float glowAlpha = 0.0f;
    if (isActive) {
        if (activeColor == juce::Colour(SkColorGetR(colors.success),
                                       SkColorGetG(colors.success),
                                       SkColorGetB(colors.success)))
            glowAlpha = playButtonGlow_;
        else
            glowAlpha = recordButtonGlow_;
    } else if (isHovered) {
        glowAlpha = stopButtonGlow_;
    }

    if (glowAlpha > 0.0f) {
        // Draw outer glow circle
        g.setColour(activeColor.withAlpha(glowAlpha * 0.3f));
        float glowRadius = radius + (glowAlpha * 8.0f);  // Glow expands with alpha
        g.fillEllipse(center.x - glowRadius, center.y - glowRadius, glowRadius * 2, glowRadius * 2);
    }

    // Main button circle
    if (isActive) {
        g.setColour(activeColor.brighter(0.1f));
    } else if (isHovered) {
        g.setColour(juce::Colour(SkColorGetR(colors.surfaceHover),
                                SkColorGetG(colors.surfaceHover),
                                SkColorGetB(colors.surfaceHover)));
    } else {
        g.setColour(juce::Colour(SkColorGetR(colors.surfaceDefault),
                                SkColorGetG(colors.surfaceDefault),
                                SkColorGetB(colors.surfaceDefault)));
    }
    g.fillEllipse(bounds);

    // Button border
    g.setColour(juce::Colour(SkColorGetR(colors.borderStrong),
                            SkColorGetG(colors.borderStrong),
                            SkColorGetB(colors.borderStrong)).withAlpha(0.6f));
    g.drawEllipse(bounds, 1.5f);

    // Inner highlight (subtle 3D effect)
    if (!isActive) {
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.fillEllipse(x + 2, y + 2, diameter - 4, diameter / 2 - 2);
    }

    // Icon/text
    g.setColour(isActive ? juce::Colours::white : activeColor);
    g.setFont(juce::Font(diameter * 0.5f));
    g.drawFittedText(icon, static_cast<int>(x), static_cast<int>(y),
                    static_cast<int>(diameter), static_cast<int>(diameter),
                    juce::Justification::centred, 1);

    // Recording pulse effect (additional outer ring)
    if (isActive && activeColor == juce::Colour(SkColorGetR(colors.danger),
                                               SkColorGetG(colors.danger),
                                               SkColorGetB(colors.danger))) {
        g.setColour(activeColor.withAlpha(recordingPulse_ * 0.5f));
        float pulseRadius = radius + 12 + (recordingPulse_ * 2);
        g.drawEllipse(center.x - pulseRadius, center.y - pulseRadius, pulseRadius * 2, pulseRadius * 2, 2.0f);
    }
}

void SkiaTransportControlComponent::drawTempoDisplay(juce::Graphics& g, int x, int y, int width, int height)
{
    const auto& colors = SkiaTheme::getInstance().getColors();
    const auto& typo = SkiaTheme::getInstance().getTypography();

    // Background
    auto bounds = juce::Rectangle<int>(x, y, width, height);
    g.setColour(juce::Colour(SkColorGetR(colors.bg3),
                            SkColorGetG(colors.bg3),
                            SkColorGetB(colors.bg3)));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Border
    g.setColour(juce::Colour(SkColorGetR(colors.borderSubtle),
                            SkColorGetG(colors.borderSubtle),
                            SkColorGetB(colors.borderSubtle)));
    g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);

    // Tempo text (large, bold, LCD-style)
    g.setColour(juce::Colour(SkColorGetR(colors.accentMain),
                            SkColorGetG(colors.accentMain),
                            SkColorGetB(colors.accentMain)));
    g.setFont(juce::Font(typo.header.size + 2, juce::Font::bold));
    g.drawFittedText(juce::String(tempo_, 1) + " BPM", bounds.reduced(4),
                    juce::Justification::centred, 1);
}

void SkiaTransportControlComponent::drawTimelineDisplay(juce::Graphics& g, int x, int y, int width, int height)
{
    const auto& colors = SkiaTheme::getInstance().getColors();
    const auto& typo = SkiaTheme::getInstance().getTypography();

    // Background
    auto bounds = juce::Rectangle<int>(x, y, width, height);
    g.setColour(juce::Colour(SkColorGetR(colors.bg2),
                            SkColorGetG(colors.bg2),
                            SkColorGetB(colors.bg2)));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Border
    g.setColour(juce::Colour(SkColorGetR(colors.borderSubtle),
                            SkColorGetG(colors.borderSubtle),
                            SkColorGetB(colors.borderSubtle)));
    g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);

    // Format time: MM:SS.cc (centiseconds)
    int minutes = static_cast<int>(timelinePosition_ / 60.0);
    int seconds = static_cast<int>(timelinePosition_) % 60;
    int centiseconds = static_cast<int>((timelinePosition_ - static_cast<int>(timelinePosition_)) * 100);
    juce::String timeStr = juce::String::formatted("%d:%02d.%02d", minutes, seconds, centiseconds);

    // LCD-style display
    g.setColour(juce::Colour(SkColorGetR(colors.accentMain),
                            SkColorGetG(colors.accentMain),
                            SkColorGetB(colors.accentMain)));
    g.setFont(juce::Font(typo.body.size + 1, juce::Font::bold)
                .withTypefaceStyle("Mono"));  // Monospace for alignment
    g.drawFittedText(timeStr, bounds.reduced(8),
                    juce::Justification::centred, 1);
}

void SkiaTransportControlComponent::resized()
{
    // Dynamically update button bounds based on actual component size
    // (This allows for responsive layouts)
}

void SkiaTransportControlComponent::mouseDown(const juce::MouseEvent& event)
{
    auto pos = event.getPosition().toFloat();

    // Play button (hit detection with circular bounds)
    if (playButtonBounds_.contains(pos))
    {
        setIsPlaying(!isPlaying_);
    }
    // Stop button
    else if (stopButtonBounds_.contains(pos))
    {
        setIsPlaying(false);
        setTimelinePosition(0.0);
    }
    // Record button
    else if (recordButtonBounds_.contains(pos))
    {
        setIsRecording(!isRecording_);
    }
}

void SkiaTransportControlComponent::mouseMove(const juce::MouseEvent& event)
{
    auto pos = event.getPosition().toFloat();

    // Update hover states
    bool wasPlayHovered = isPlayHovered_;
    bool wasStopHovered = isStopHovered_;
    bool wasRecordHovered = isRecordHovered_;

    isPlayHovered_ = playButtonBounds_.contains(pos);
    isStopHovered_ = stopButtonBounds_.contains(pos);
    isRecordHovered_ = recordButtonBounds_.contains(pos);

    // Only repaint if hover state changed
    if (wasPlayHovered != isPlayHovered_ || wasStopHovered != isStopHovered_ ||
        wasRecordHovered != isRecordHovered_)
    {
        repaint();
    }
}

void SkiaTransportControlComponent::mouseExit(const juce::MouseEvent& event)
{
    // Clear all hover states
    if (isPlayHovered_ || isStopHovered_ || isRecordHovered_)
    {
        isPlayHovered_ = false;
        isStopHovered_ = false;
        isRecordHovered_ = false;
        repaint();
    }
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
