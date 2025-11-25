/**
 * @file TransportControlComponent.cpp
 * @brief Transport control implementation with animations
 * 
 * DESIGN SYSTEM: Updated to use ZenithLookAndFeel design tokens
 */

#include "TransportControlComponent.h"
#include "../../include/Engine.h"
#include "ZenithLookAndFeel.h"  // DESIGN SYSTEM: Include for design tokens

namespace zenith {

//==============================================================================
TransportControlComponent::TransportControlComponent(Engine& eng)
    : engine_(eng)
{
    setSize(400, 120);
    startTimer(16);  // 60 Hz refresh rate for smooth animations
}

TransportControlComponent::~TransportControlComponent()
{
    stopTimer();
}

//==============================================================================
void TransportControlComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // DESIGN SYSTEM: Background with subtle gradient using elevation tokens
    juce::ColourGradient bgGradient(
        juce::Colour(ZenithLookAndFeel::Elevation::dp4), 0.0f, (float)bounds.getY(),
        juce::Colour(ZenithLookAndFeel::Elevation::dp1), 0.0f, (float)bounds.getBottom(),
        false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    // DESIGN SYSTEM: Border using borderSubtle token
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::borderSubtle));
    g.drawRect(bounds, 1);

    // Center content area
    auto centerArea = bounds.removeFromTop(60);
    centerArea = centerArea.withSizeKeepingCentre(centerArea.getWidth() - SPACING * 2, BUTTON_SIZE + SPACING);

    // Layout buttons horizontally centered
    int totalButtonWidth = (BUTTON_SIZE * 3) + (BUTTON_SPACING * 2);
    int startX = centerArea.getCentreX() - (totalButtonWidth / 2);

    // DESIGN SYSTEM: Play button with playGreen semantic color
    auto playBounds = juce::Rectangle<int>(startX, centerArea.getCentreY() - BUTTON_SIZE / 2,
                                           BUTTON_SIZE, BUTTON_SIZE);
    drawButton(g, playBounds, juce::CharPointer_UTF8("\xe2\x96\xb6"), // Play triangle
               juce::Colour(ZenithLookAndFeel::Colors::playGreen),
               playPressed_, playHovered_, engine_.isPlaying());

    // DESIGN SYSTEM: Stop button with accentPrimary color  
    auto stopBounds = juce::Rectangle<int>(startX + BUTTON_SIZE + BUTTON_SPACING,
                                          centerArea.getCentreY() - BUTTON_SIZE / 2,
                                          BUTTON_SIZE, BUTTON_SIZE);
    drawButton(g, stopBounds, juce::CharPointer_UTF8("\xe2\x8f\xb9"), // Stop square
               juce::Colour(ZenithLookAndFeel::Colors::accentPrimary),
               stopPressed_, stopHovered_, !engine_.isPlaying() && !engine_.isRecording());

    // DESIGN SYSTEM: Record button with recordRed semantic color
    auto recordBounds = juce::Rectangle<int>(startX + (BUTTON_SIZE + BUTTON_SPACING) * 2,
                                            centerArea.getCentreY() - BUTTON_SIZE / 2,
                                            BUTTON_SIZE, BUTTON_SIZE);
    drawButton(g, recordBounds, juce::CharPointer_UTF8("\xe2\x97\x8f"), // Record circle
               juce::Colour(ZenithLookAndFeel::Colors::recordRed),
               recordPressed_, recordHovered_, engine_.isRecording());

    // Status and time display below buttons
    auto statusArea = bounds.removeFromTop(40);
    statusArea = statusArea.reduced(SPACING);

    // DESIGN SYSTEM: Status text with textSecondary token
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
    g.setFont(ZenithLookAndFeel::Typography::getSmallBold());

    juce::String statusText = engine_.isRecording() ? juce::CharPointer_UTF8("\xe2\x97\x8f RECORDING")
                            : engine_.isPlaying() ? juce::CharPointer_UTF8("\xe2\x96\xb6 PLAYING")
                            : juce::CharPointer_UTF8("\xe2\x8f\xb9 STOPPED");

    // DESIGN SYSTEM: Status color using semantic colors
    juce::Colour statusColor = engine_.isRecording() ? juce::Colour(ZenithLookAndFeel::Colors::recordRed)
                             : engine_.isPlaying() ? juce::Colour(ZenithLookAndFeel::Colors::playGreen)
                             : juce::Colour(ZenithLookAndFeel::Colors::textSecondary);
    g.setColour(statusColor);
    g.drawText(statusText, statusArea.removeFromLeft(120), juce::Justification::centredLeft, true);

    // DESIGN SYSTEM: Playback time with textPrimary token
    juce::String timeText = formatTime(engine_.getPlayheadSamples());
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    g.setFont(ZenithLookAndFeel::Typography::getSmall());
    g.drawText(timeText, statusArea, juce::Justification::centredRight, true);
}

void TransportControlComponent::resized()
{
    // Layout handled in paint()
}

void TransportControlComponent::timerCallback()
{
    updateAnimationState();
    repaint();
}

//==============================================================================
void TransportControlComponent::drawButton(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                                          const juce::String& label, const juce::Colour& color,
                                          bool isPressed, bool isHovered, bool isActive)
{
    auto buttonBounds = bounds.toFloat();

    // Button background with scale animation
    float scale = isPressed ? 0.92f : (isHovered ? 1.08f : 1.0f);
    auto scaledBounds = buttonBounds.withSizeKeepingCentre(
        buttonBounds.getWidth() * scale,
        buttonBounds.getHeight() * scale);

    // DESIGN SYSTEM: Draw outer shadow using elevation
    if (!isPressed) {
        g.setColour(juce::Colour(ZenithLookAndFeel::Elevation::dp0).withAlpha(0.3f));
        g.drawEllipse(scaledBounds.expanded(2.0f), 1.0f);
    }

    // Button gradient
    juce::ColourGradient buttonGradient(
        color.brighter(0.3f), scaledBounds.getCentreX(), scaledBounds.getY(),
        color.darker(0.2f), scaledBounds.getCentreX(), scaledBounds.getBottom(),
        false);
    g.setGradientFill(buttonGradient);
    g.fillEllipse(scaledBounds);

    // Button border
    g.setColour(isActive ? color.brighter(0.5f) : color.withAlpha(0.5f));
    g.drawEllipse(scaledBounds, isActive ? 2.5f : 1.5f);

    // DESIGN SYSTEM: Highlight on top using textPrimary with alpha
    if (isActive) {
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary).withAlpha(0.2f));
        g.fillEllipse(scaledBounds.withHeight(scaledBounds.getHeight() * 0.4f));
    }

    // DESIGN SYSTEM: Label text using textPrimary
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    g.setFont(ZenithLookAndFeel::Typography::getH2());
    g.drawText(label, bounds, juce::Justification::centred, true);

    // DESIGN SYSTEM: Recording pulse effect with recordRed
    if (isActive && label.containsChar(0x25CF)) { // Record circle character
        juce::Colour pulseColor = color.withAlpha(0.3f + 0.2f * std::sin(recordPulseAnimation_ * juce::MathConstants<float>::twoPi));
        g.setColour(pulseColor);
        g.drawEllipse(scaledBounds.expanded(4.0f), 2.0f);
    }
}

//==============================================================================
juce::Rectangle<int> TransportControlComponent::getPlayButtonBounds() const
{
    auto bounds = getLocalBounds().removeFromTop(60);
    int centerY = bounds.getCentreY();
    int totalButtonWidth = (BUTTON_SIZE * 3) + (BUTTON_SPACING * 2);
    int startX = bounds.getCentreX() - (totalButtonWidth / 2);

    return juce::Rectangle<int>(startX, centerY - BUTTON_SIZE / 2, BUTTON_SIZE, BUTTON_SIZE);
}

juce::Rectangle<int> TransportControlComponent::getStopButtonBounds() const
{
    auto bounds = getLocalBounds().removeFromTop(60);
    int centerY = bounds.getCentreY();
    int totalButtonWidth = (BUTTON_SIZE * 3) + (BUTTON_SPACING * 2);
    int startX = bounds.getCentreX() - (totalButtonWidth / 2);

    return juce::Rectangle<int>(startX + BUTTON_SIZE + BUTTON_SPACING,
                               centerY - BUTTON_SIZE / 2, BUTTON_SIZE, BUTTON_SIZE);
}

juce::Rectangle<int> TransportControlComponent::getRecordButtonBounds() const
{
    auto bounds = getLocalBounds().removeFromTop(60);
    int centerY = bounds.getCentreY();
    int totalButtonWidth = (BUTTON_SIZE * 3) + (BUTTON_SPACING * 2);
    int startX = bounds.getCentreX() - (totalButtonWidth / 2);

    return juce::Rectangle<int>(startX + (BUTTON_SIZE + BUTTON_SPACING) * 2,
                               centerY - BUTTON_SIZE / 2, BUTTON_SIZE, BUTTON_SIZE);
}

//==============================================================================
void TransportControlComponent::mouseDown(const juce::MouseEvent& event)
{
    if (getPlayButtonBounds().contains(event.getPosition())) {
        playPressed_ = true;
        handlePlayButtonClick();
    } else if (getStopButtonBounds().contains(event.getPosition())) {
        stopPressed_ = true;
        handleStopButtonClick();
    } else if (getRecordButtonBounds().contains(event.getPosition())) {
        recordPressed_ = true;
        handleRecordButtonClick();
    }
}

void TransportControlComponent::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    playPressed_ = false;
    stopPressed_ = false;
    recordPressed_ = false;
}

void TransportControlComponent::mouseDrag(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
}

//==============================================================================
void TransportControlComponent::handlePlayButtonClick()
{
    if (!engine_.isPlaying()) {
        engine_.play();
    } else {
        engine_.stop();
    }
}

void TransportControlComponent::handleStopButtonClick()
{
    engine_.stop();
}

void TransportControlComponent::handleRecordButtonClick()
{
    if (!engine_.isRecording()) {
        engine_.record();
    } else {
        engine_.stopRecording();
    }
}

//==============================================================================
void TransportControlComponent::updateAnimationState()
{
    // Pulse animation for recording indicator
    recordPulseAnimation_ += 0.02f;
    if (recordPulseAnimation_ > 1.0f) {
        recordPulseAnimation_ -= 1.0f;
    }

    // Button scale animations (spring-like)
    const float TARGET_SCALE = 1.0f;
    const float SPRING_CONSTANT = 0.15f;

    playButtonScale_ += (TARGET_SCALE - playButtonScale_) * SPRING_CONSTANT;
    stopButtonScale_ += (TARGET_SCALE - stopButtonScale_) * SPRING_CONSTANT;
    recordButtonScale_ += (TARGET_SCALE - recordButtonScale_) * SPRING_CONSTANT;
}

juce::String TransportControlComponent::formatTime(juce::int64 samples)
{
    double sampleRate = 44100.0;  // TODO(zenith-core#1): Get from engine
    double seconds = (double)samples / sampleRate;

    int hours = (int)(seconds / 3600.0);
    seconds -= hours * 3600.0;

    int minutes = (int)(seconds / 60.0);
    seconds -= minutes * 60.0;

    int secs = (int)seconds;
    int ms = (int)((seconds - secs) * 1000.0);

    return juce::String::formatted("%02d:%02d:%02d.%03d", hours, minutes, secs, ms);
}

}  // namespace zenith
