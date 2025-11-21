/**
 * @file SkiaMixerChannelComponent.cpp
 * @brief Implementation of Skia mixer channel component
 */

#include "SkiaMixerChannelComponent.h"

#ifdef ZENITH_USE_SKIA

#include <cmath>

namespace zenith {

SkiaMixerChannelComponent::SkiaMixerChannelComponent()
    : faderValue_(0.7f), faderTarget_(0.7f), panValue_(0.0f), panTarget_(0.0f),
      isMuted_(false), isSolo_(false), inputLevel_(0.0f), inputLevelSmoothed_(0.0f),
      peakLevel_(0.0f)
{
    setSize(60, 300);
    startTimer(16); // ~60 FPS
}

SkiaMixerChannelComponent::~SkiaMixerChannelComponent()
{
    stopTimer();
}

void SkiaMixerChannelComponent::setFaderValue(float value)
{
    faderTarget_ = juce::jlimit(0.0f, 1.0f, value);
}

void SkiaMixerChannelComponent::setPanValue(float value)
{
    panTarget_ = juce::jlimit(-1.0f, 1.0f, value);
}

void SkiaMixerChannelComponent::setMuted(bool muted)
{
    if (isMuted_ != muted)
    {
        isMuted_ = muted;
        repaint();
    }
}

void SkiaMixerChannelComponent::setSolo(bool solo)
{
    if (isSolo_ != solo)
    {
        isSolo_ = solo;
        repaint();
    }
}

void SkiaMixerChannelComponent::setInputLevel(float level)
{
    inputLevel_ = juce::jlimit(0.0f, 1.0f, level);

    // Smooth the level
    const float smoothingFactor = 0.15f;
    inputLevelSmoothed_ += (inputLevel_ - inputLevelSmoothed_) * smoothingFactor;

    // Track peak
    if (inputLevelSmoothed_ > peakLevel_)
    {
        peakLevel_ = inputLevelSmoothed_;
        meter_.peakHoldSamples = 60; // Hold peak for 1 second at 60 FPS
    }
}

void SkiaMixerChannelComponent::setChannelName(const juce::String& name)
{
    channelName_ = name;
    repaint();
}

void SkiaMixerChannelComponent::paint(juce::Graphics& g)
{
    const auto& colors = SkiaTheme::getInstance().getColors();
    const auto& depthStyle = SkiaTheme::getInstance().getDepthStyle();

    // Background
    g.fillAll(juce::Colour(
        colors.backgroundSecondary >> 16 & 0xFF,
        colors.backgroundSecondary >> 8 & 0xFF,
        colors.backgroundSecondary & 0xFF
    ));

    // Border
    g.setColour(juce::Colour(
        colors.border >> 16 & 0xFF,
        colors.border >> 8 & 0xFF,
        colors.border & 0xFF
    ));
    g.drawRect(getLocalBounds(), 1);

    renderLabel();
    renderFader();
    renderPanKnob();
    renderMeterDisplay();
    renderButtons();
}

void SkiaMixerChannelComponent::resized()
{
    // Layout:
    // Top: Channel name label
    // Middle: Fader + meter
    // Bottom: Pan knob + Mute/Solo buttons
}

void SkiaMixerChannelComponent::mouseDown(const juce::MouseEvent& event)
{
    currentHitTarget_ = getHitTarget(event.getPosition());

    if (currentHitTarget_ == HitTarget::Fader)
    {
        isDraggingFader_ = true;
    }
    else if (currentHitTarget_ == HitTarget::Pan)
    {
        isDraggingPan_ = true;
    }
    else if (currentHitTarget_ == HitTarget::Mute)
    {
        setMuted(!isMuted_);
    }
    else if (currentHitTarget_ == HitTarget::Solo)
    {
        setSolo(!isSolo_);
    }
}

void SkiaMixerChannelComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isDraggingFader_)
    {
        float newValue = 1.0f - (event.y / static_cast<float>(getHeight()));
        setFaderValue(newValue);
    }
    else if (isDraggingPan_)
    {
        float newValue = (event.x / static_cast<float>(getWidth())) * 2.0f - 1.0f;
        setPanValue(newValue);
    }
}

void SkiaMixerChannelComponent::mouseUp(const juce::MouseEvent& event)
{
    isDraggingFader_ = false;
    isDraggingPan_ = false;
    currentHitTarget_ = HitTarget::None;
}

void SkiaMixerChannelComponent::timerCallback()
{
    // Smooth fader animation
    const float smoothFactor = 0.2f;
    faderValue_ += (faderTarget_ - faderValue_) * smoothFactor;

    // Smooth pan animation
    panValue_ += (panTarget_ - panValue_) * smoothFactor;

    // Decay peak hold
    if (meter_.peakHoldSamples > 0)
    {
        meter_.peakHoldSamples--;
    }
    else if (peakLevel_ > inputLevelSmoothed_)
    {
        peakLevel_ *= 0.95f; // Decay peak
    }

    repaint();
}

void SkiaMixerChannelComponent::renderFader()
{
    // Vertical fader from bottom to top
    const int faderWidth = 20;
    const int faderHeight = getHeight() - 100;
    const int faderX = (getWidth() - faderWidth) / 2;
    const int faderY = 50;

    // Track background
    auto bounds = getLocalBounds().withHeight(faderHeight).withX(faderX).withY(faderY);
}

void SkiaMixerChannelComponent::renderPanKnob()
{
    // Circular pan knob at bottom
}

void SkiaMixerChannelComponent::renderMeterDisplay()
{
    // Level meter on the right side
}

void SkiaMixerChannelComponent::renderButtons()
{
    // Mute and Solo buttons
}

void SkiaMixerChannelComponent::renderLabel()
{
    // Channel name at top
}

SkiaMixerChannelComponent::HitTarget SkiaMixerChannelComponent::getHitTarget(
    const juce::Point<int>& pos) const
{
    // Determine what was clicked
    const int height = getHeight();
    const int width = getWidth();

    // Button area at bottom (last 40 pixels)
    if (pos.y > height - 40)
    {
        if (pos.x < width / 2)
            return HitTarget::Mute;
        else
            return HitTarget::Solo;
    }

    // Pan knob area (bottom middle)
    if (pos.y > height - 80)
    {
        return HitTarget::Pan;
    }

    // Fader area (middle)
    return HitTarget::Fader;
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
