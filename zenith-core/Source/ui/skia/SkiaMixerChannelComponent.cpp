/**
 * @file SkiaMixerChannelComponent.cpp
 * @brief Implementation of Skia mixer channel component with full GPU rendering
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

    // Background with gradient
    g.fillAll(juce::Colour(SkColorGetR(colors.backgroundSecondary),
                           SkColorGetG(colors.backgroundSecondary),
                           SkColorGetB(colors.backgroundSecondary)));

    // Subtle gradient overlay for depth
    auto bounds = getLocalBounds();
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(SkColorGetR(colors.backgroundTertiary),
                     SkColorGetG(colors.backgroundTertiary),
                     SkColorGetB(colors.backgroundTertiary)).withAlpha(0.3f),
        0, 0,
        juce::Colour(SkColorGetR(colors.backgroundSecondary),
                     SkColorGetG(colors.backgroundSecondary),
                     SkColorGetB(colors.backgroundSecondary)),
        0, static_cast<float>(bounds.getHeight()),
        false));
    g.fillRect(bounds);

    // Border with slight glow
    g.setColour(juce::Colour(SkColorGetR(colors.border),
                             SkColorGetG(colors.border),
                             SkColorGetB(colors.border)));
    g.drawRect(bounds, 1);

    // Render all components
    renderLabel(g);
    renderMeterDisplay(g);
    renderFader(g);
    renderPanKnob(g);
    renderButtons(g);
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
        float faderHeight = static_cast<float>(getHeight() - 140);
        float faderY = 30.0f;
        float newValue = 1.0f - ((event.y - faderY) / faderHeight);
        setFaderValue(newValue);
    }
    else if (isDraggingPan_)
    {
        int knobY = getHeight() - 90;
        int knobSize = 32;
        juce::Point<float> center(static_cast<float>(getWidth() / 2), static_cast<float>(knobY + knobSize / 2));
        float dx = event.x - center.x;
        float dy = event.y - center.y;
        float angle = std::atan2(dy, dx) * 180.0f / juce::MathConstants<float>::pi - 90.0f;

        // Map -135° to +135° range to -1 to +1
        float newValue = juce::jlimit(-1.0f, 1.0f, angle / 135.0f);
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
    // Spring physics for smooth animations
    const auto& physics = SkiaTheme::getInstance().getSliderPhysics();
    const float dt = 1.0f / physics.fps;
    bool needsRepaint = false;

    // Smooth fader animation
    if (std::abs(faderValue_ - faderTarget_) > 0.001f)
    {
        const float smoothFactor = 0.2f;
        faderValue_ += (faderTarget_ - faderValue_) * smoothFactor;
        needsRepaint = true;
    }

    // Smooth pan animation
    if (std::abs(panValue_ - panTarget_) > 0.001f)
    {
        const float smoothFactor = 0.2f;
        panValue_ += (panTarget_ - panValue_) * smoothFactor;
        needsRepaint = true;
    }

    // Decay peak hold
    if (meter_.peakHoldSamples > 0)
    {
        meter_.peakHoldSamples--;
    }
    else if (peakLevel_ > inputLevelSmoothed_)
    {
        peakLevel_ *= 0.95f; // Decay peak
        needsRepaint = true;
    }

    if (needsRepaint)
    {
        repaint();
    }
}

//==============================================================================
// Rendering Methods
//==============================================================================

void SkiaMixerChannelComponent::renderLabel(juce::Graphics& g)
{
    const auto& colors = SkiaTheme::getInstance().getColors();
    const auto& typo = SkiaTheme::getInstance().getTypography();

    // Channel name at top with subtle background
    auto labelBounds = juce::Rectangle<int>(0, 0, getWidth(), 24);

    g.setColour(juce::Colour(SkColorGetR(colors.backgroundTertiary),
                             SkColorGetG(colors.backgroundTertiary),
                             SkColorGetB(colors.backgroundTertiary)));
    g.fillRect(labelBounds);

    // Text
    g.setColour(juce::Colour(SkColorGetR(colors.textPrimary),
                             SkColorGetG(colors.textPrimary),
                             SkColorGetB(colors.textPrimary)));
    g.setFont(juce::Font(typo.smallSize, juce::Font::bold));
    g.drawText(channelName_, labelBounds, juce::Justification::centred);
}

void SkiaMixerChannelComponent::renderFader(juce::Graphics& g)
{
    const auto& colors = SkiaTheme::getInstance().getColors();

    // Vertical fader from bottom to top
    const int faderWidth = 8;
    const int faderHeight = getHeight() - 140;
    const int faderX = 12;
    const int faderY = 30;

    // Track background with rounded corners
    auto trackBounds = juce::Rectangle<float>(static_cast<float>(faderX),
                                              static_cast<float>(faderY),
                                              static_cast<float>(faderWidth),
                                              static_cast<float>(faderHeight));

    g.setColour(juce::Colour(SkColorGetR(colors.surfaceDefault),
                             SkColorGetG(colors.surfaceDefault),
                             SkColorGetB(colors.surfaceDefault)));
    g.fillRoundedRectangle(trackBounds, 4.0f);

    // Filled portion (value)
    float fillHeight = faderValue_ * faderHeight;
    auto fillBounds = juce::Rectangle<float>(static_cast<float>(faderX),
                                             static_cast<float>(faderY + faderHeight - fillHeight),
                                             static_cast<float>(faderWidth),
                                             fillHeight);

    // Gradient fill
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(SkColorGetR(colors.primary),
                     SkColorGetG(colors.primary),
                     SkColorGetB(colors.primary)),
        fillBounds.getCentreX(), fillBounds.getBottom(),
        juce::Colour(SkColorGetR(colors.primaryHover),
                     SkColorGetG(colors.primaryHover),
                     SkColorGetB(colors.primaryHover)).brighter(0.2f),
        fillBounds.getCentreX(), fillBounds.getY(),
        false));
    g.fillRoundedRectangle(fillBounds, 4.0f);

    // Fader cap (thumb)
    float capY = faderY + (1.0f - faderValue_) * faderHeight;
    auto capBounds = juce::Rectangle<float>(static_cast<float>(faderX - 4),
                                           capY - 6.0f,
                                           static_cast<float>(faderWidth + 8),
                                           12.0f);

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillRoundedRectangle(capBounds.translated(0, 2), 2.0f);

    // Cap body with gradient
    g.setGradientFill(juce::ColourGradient(
        juce::Colours::white,
        capBounds.getCentreX(), capBounds.getY(),
        juce::Colours::lightgrey,
        capBounds.getCentreX(), capBounds.getBottom(),
        false));
    g.fillRoundedRectangle(capBounds, 2.0f);

    // Cap border
    g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));
    g.drawRoundedRectangle(capBounds, 2.0f, 1.0f);
}

void SkiaMixerChannelComponent::renderPanKnob(juce::Graphics& g)
{
    const auto& colors = SkiaTheme::getInstance().getColors();

    // Pan knob at bottom middle area
    const int knobSize = 32;
    const int knobX = 8;
    const int knobY = getHeight() - 90;

    auto knobBounds = juce::Rectangle<float>(static_cast<float>(knobX),
                                            static_cast<float>(knobY),
                                            static_cast<float>(knobSize),
                                            static_cast<float>(knobSize));

    // Knob shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillEllipse(knobBounds.translated(0, 2));

    // Knob body with radial gradient
    juce::Point<float> center = knobBounds.getCentre();
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(80, 80, 85),
        center.x, center.y - knobSize * 0.3f,
        juce::Colour(40, 40, 45),
        center.x, center.y + knobSize * 0.3f,
        true));
    g.fillEllipse(knobBounds);

    // Pan indicator line
    float angle = panValue_ * 135.0f; // -135° to +135°
    float angleRad = (angle + 90.0f) * juce::MathConstants<float>::pi / 180.0f;
    float indicatorLen = knobSize / 2.5f;

    juce::Point<float> indicatorEnd(
        center.x + std::cos(angleRad) * indicatorLen,
        center.y + std::sin(angleRad) * indicatorLen
    );

    g.setColour(juce::Colours::white);
    g.drawLine(center.x, center.y, indicatorEnd.x, indicatorEnd.y, 2.0f);

    // Center dot
    g.fillEllipse(center.x - 2, center.y - 2, 4, 4);

    // Pan label
    g.setColour(juce::Colour(SkColorGetR(colors.textSecondary),
                             SkColorGetG(colors.textSecondary),
                             SkColorGetB(colors.textSecondary)));
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("PAN", knobX, knobY + knobSize + 2, knobSize, 12,
               juce::Justification::centred);
}

void SkiaMixerChannelComponent::renderMeterDisplay(juce::Graphics& g)
{
    const auto& colors = SkiaTheme::getInstance().getColors();

    // VU meter on the right side
    const int meterWidth = 12;
    const int meterHeight = getHeight() - 140;
    const int meterX = getWidth() - meterWidth - 6;
    const int meterY = 30;

    // Meter background
    g.setColour(juce::Colour(30, 30, 30));
    g.fillRoundedRectangle(static_cast<float>(meterX),
                          static_cast<float>(meterY),
                          static_cast<float>(meterWidth),
                          static_cast<float>(meterHeight),
                          3.0f);

    // Level fill with color zones
    float levelHeight = inputLevelSmoothed_ * meterHeight;

    if (levelHeight > 0.0f)
    {
        // Green zone (0-70%)
        float greenHeight = std::min(levelHeight, meterHeight * 0.7f);
        if (greenHeight > 0)
        {
            auto greenBounds = juce::Rectangle<float>(
                static_cast<float>(meterX),
                static_cast<float>(meterY + meterHeight - greenHeight),
                static_cast<float>(meterWidth),
                greenHeight
            );

            g.setGradientFill(juce::ColourGradient(
                juce::Colour(SkColorGetR(colors.meterGreen),
                             SkColorGetG(colors.meterGreen),
                             SkColorGetB(colors.meterGreen)),
                greenBounds.getCentreX(), greenBounds.getBottom(),
                juce::Colour(SkColorGetR(colors.meterGreen),
                             SkColorGetG(colors.meterGreen),
                             SkColorGetB(colors.meterGreen)).darker(0.3f),
                greenBounds.getCentreX(), greenBounds.getY(),
                false));
            g.fillRoundedRectangle(greenBounds, 3.0f);
        }

        // Yellow zone (70-90%)
        if (levelHeight > meterHeight * 0.7f)
        {
            float yellowHeight = std::min(levelHeight - meterHeight * 0.7f,
                                         meterHeight * 0.2f);
            auto yellowBounds = juce::Rectangle<float>(
                static_cast<float>(meterX),
                static_cast<float>(meterY + meterHeight * 0.1f),
                static_cast<float>(meterWidth),
                yellowHeight
            );

            g.setColour(juce::Colour(SkColorGetR(colors.meterYellow),
                                     SkColorGetG(colors.meterYellow),
                                     SkColorGetB(colors.meterYellow)));
            g.fillRoundedRectangle(yellowBounds, 3.0f);
        }

        // Red zone (90-100%)
        if (levelHeight > meterHeight * 0.9f)
        {
            float redHeight = levelHeight - meterHeight * 0.9f;
            auto redBounds = juce::Rectangle<float>(
                static_cast<float>(meterX),
                static_cast<float>(meterY),
                static_cast<float>(meterWidth),
                redHeight
            );

            g.setColour(juce::Colour(SkColorGetR(colors.meterRed),
                                     SkColorGetG(colors.meterRed),
                                     SkColorGetB(colors.meterRed)));
            g.fillRoundedRectangle(redBounds, 3.0f);
        }
    }

    // Peak indicator
    if (peakLevel_ > 0.01f)
    {
        float peakY = meterY + (1.0f - peakLevel_) * meterHeight;
        g.setColour(juce::Colours::white);
        g.fillRect(static_cast<float>(meterX), peakY, static_cast<float>(meterWidth), 2.0f);
    }

    // Meter border
    g.setColour(juce::Colour(60, 60, 60));
    g.drawRoundedRectangle(static_cast<float>(meterX),
                          static_cast<float>(meterY),
                          static_cast<float>(meterWidth),
                          static_cast<float>(meterHeight),
                          3.0f, 1.0f);
}

void SkiaMixerChannelComponent::renderButtons(juce::Graphics& g)
{
    const auto& colors = SkiaTheme::getInstance().getColors();

    // Mute and Solo buttons at the bottom
    const int buttonWidth = 22;
    const int buttonHeight = 18;
    const int buttonY = getHeight() - 50;
    const int muteX = 4;
    const int soloX = 28;

    // Mute button
    auto muteBounds = juce::Rectangle<float>(static_cast<float>(muteX),
                                            static_cast<float>(buttonY),
                                            static_cast<float>(buttonWidth),
                                            static_cast<float>(buttonHeight));

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(muteBounds.translated(0, 1), 3.0f);

    // Button fill
    if (isMuted_)
    {
        g.setColour(juce::Colour(SkColorGetR(colors.warning),
                                 SkColorGetG(colors.warning),
                                 SkColorGetB(colors.warning)));
    }
    else
    {
        g.setColour(juce::Colour(SkColorGetR(colors.surfaceDefault),
                                 SkColorGetG(colors.surfaceDefault),
                                 SkColorGetB(colors.surfaceDefault)));
    }
    g.fillRoundedRectangle(muteBounds, 3.0f);

    // Border
    g.setColour(juce::Colour(SkColorGetR(colors.border),
                             SkColorGetG(colors.border),
                             SkColorGetB(colors.border)));
    g.drawRoundedRectangle(muteBounds, 3.0f, 1.0f);

    // Text
    g.setColour(isMuted_ ? juce::Colours::white :
                juce::Colour(SkColorGetR(colors.textPrimary),
                             SkColorGetG(colors.textPrimary),
                             SkColorGetB(colors.textPrimary)));
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("M", muteBounds.toNearestInt(), juce::Justification::centred);

    // Solo button
    auto soloBounds = juce::Rectangle<float>(static_cast<float>(soloX),
                                            static_cast<float>(buttonY),
                                            static_cast<float>(buttonWidth),
                                            static_cast<float>(buttonHeight));

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(soloBounds.translated(0, 1), 3.0f);

    // Button fill
    if (isSolo_)
    {
        g.setColour(juce::Colour(SkColorGetR(colors.success),
                                 SkColorGetG(colors.success),
                                 SkColorGetB(colors.success)));
    }
    else
    {
        g.setColour(juce::Colour(SkColorGetR(colors.surfaceDefault),
                                 SkColorGetG(colors.surfaceDefault),
                                 SkColorGetB(colors.surfaceDefault)));
    }
    g.fillRoundedRectangle(soloBounds, 3.0f);

    // Border
    g.setColour(juce::Colour(SkColorGetR(colors.border),
                             SkColorGetG(colors.border),
                             SkColorGetB(colors.border)));
    g.drawRoundedRectangle(soloBounds, 3.0f, 1.0f);

    // Text
    g.setColour(isSolo_ ? juce::Colours::white :
                juce::Colour(SkColorGetR(colors.textPrimary),
                             SkColorGetG(colors.textPrimary),
                             SkColorGetB(colors.textPrimary)));
    g.drawText("S", soloBounds.toNearestInt(), juce::Justification::centred);
}

SkiaMixerChannelComponent::HitTarget SkiaMixerChannelComponent::getHitTarget(
    const juce::Point<int>& pos) const
{
    // Determine what was clicked
    const int height = getHeight();
    const int width = getWidth();

    // Button area at bottom (last 60 pixels)
    if (pos.y > height - 60)
    {
        if (pos.y > height - 50)
        {
            if (pos.x < width / 2)
                return HitTarget::Mute;
            else
                return HitTarget::Solo;
        }
        // Pan knob area
        return HitTarget::Pan;
    }

    // Meter is on the right side
    if (pos.x > width - 18)
    {
        return HitTarget::None; // Meter is not interactive
    }

    // Fader area (left/middle)
    return HitTarget::Fader;
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
