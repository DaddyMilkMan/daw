/**
 * @file MixerChannelComponent.cpp
 * @brief Mixer channel strip implementation
 */

#include "../../include/ui/MixerChannelComponent.h"
#include "../../Source/engine/Track.h"

//==============================================================================
MixerChannelComponent::MixerChannelComponent(zenith::Track* track)
    : track_(track)
{
    jassert(track_ != nullptr);

    // Track name label
    nameLabel_.setText(track_->getName(), juce::dontSendNotification);
    nameLabel_.setJustificationType(juce::Justification::centred);
    nameLabel_.setFont(juce::Font(14.0f, juce::Font::bold));
    addAndMakeVisible(nameLabel_);

    // Volume fader (vertical slider)
    faderSlider_.setSliderStyle(juce::Slider::LinearVertical);
    faderSlider_.setRange(0.0, 1.0, 0.001);
    faderSlider_.setValue(track_->getVolume(), juce::dontSendNotification);
    faderSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    faderSlider_.setNumDecimalPlacesToDisplay(2);
    faderSlider_.onValueChange = [this]() { onFaderChanged(); };
    addAndMakeVisible(faderSlider_);

    // Pan control (rotary knob)
    panSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider_.setRange(-1.0, 1.0, 0.01);
    panSlider_.setValue(track_->getPan(), juce::dontSendNotification);
    panSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    panSlider_.setNumDecimalPlacesToDisplay(2);
    panSlider_.onValueChange = [this]() { onPanChanged(); };
    addAndMakeVisible(panSlider_);

    // Mute button
    muteButton_.setButtonText("M");
    muteButton_.setClickingTogglesState(true);
    muteButton_.setToggleState(track_->isMuted(), juce::dontSendNotification);
    muteButton_.onClick = [this]() { onMuteClicked(); };
    addAndMakeVisible(muteButton_);

    // Solo button
    soloButton_.setButtonText("S");
    soloButton_.setClickingTogglesState(true);
    soloButton_.setToggleState(track_->isSolo(), juce::dontSendNotification);
    soloButton_.onClick = [this]() { onSoloClicked(); };
    addAndMakeVisible(soloButton_);

    // Level meter
    addAndMakeVisible(meter_);

    // Start timer for meter updates (30 Hz)
    startTimer(33);

    setSize(80, 400);
}

MixerChannelComponent::~MixerChannelComponent()
{
    stopTimer();
}

//==============================================================================
void MixerChannelComponent::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(getLocalBounds(), 1);
}

void MixerChannelComponent::resized()
{
    auto bounds = getLocalBounds().reduced(5);

    // Track name at top
    nameLabel_.setBounds(bounds.removeFromTop(25));

    // Mute/Solo buttons at bottom
    auto buttonArea = bounds.removeFromBottom(60);
    muteButton_.setBounds(buttonArea.removeFromTop(28).reduced(2));
    soloButton_.setBounds(buttonArea.removeFromTop(28).reduced(2));

    // Pan control above buttons
    auto panArea = bounds.removeFromBottom(80);
    panSlider_.setBounds(panArea);

    // Split remaining space between meter and fader
    auto meterBounds = bounds.removeFromLeft(15);
    meter_.setBounds(meterBounds.reduced(0, 5));

    // Fader takes remaining space
    faderSlider_.setBounds(bounds.reduced(5, 5));
}

//==============================================================================
void MixerChannelComponent::timerCallback()
{
    // Update meter from track level (thread-safe read via atomic)
    if (track_ != nullptr)
    {
        float level = track_->getCurrentLevel();
        meter_.setLevel(level);
        meter_.repaint();
    }
}

void MixerChannelComponent::updateFromTrack()
{
    if (track_ == nullptr)
        return;

    updatingControls_ = true;

    // Update fader
    faderSlider_.setValue(track_->getVolume(), juce::dontSendNotification);

    // Update pan
    panSlider_.setValue(track_->getPan(), juce::dontSendNotification);

    // Update mute
    muteButton_.setToggleState(track_->isMuted(), juce::dontSendNotification);

    // Update solo
    soloButton_.setToggleState(track_->isSolo(), juce::dontSendNotification);

    // Update name
    nameLabel_.setText(track_->getName(), juce::dontSendNotification);

    updatingControls_ = false;
}

//==============================================================================
void MixerChannelComponent::onFaderChanged()
{
    if (updatingControls_ || track_ == nullptr)
        return;

    // Update track volume (thread-safe via atomic)
    float newVolume = static_cast<float>(faderSlider_.getValue());
    track_->setVolume(newVolume);
}

void MixerChannelComponent::onPanChanged()
{
    if (updatingControls_ || track_ == nullptr)
        return;

    // Update track pan (thread-safe via atomic)
    float newPan = static_cast<float>(panSlider_.getValue());
    track_->setPan(newPan);
}

void MixerChannelComponent::onMuteClicked()
{
    if (updatingControls_ || track_ == nullptr)
        return;

    // Toggle mute (thread-safe via atomic)
    track_->setMuted(muteButton_.getToggleState());
}

void MixerChannelComponent::onSoloClicked()
{
    if (updatingControls_ || track_ == nullptr)
        return;

    // Toggle solo (thread-safe via atomic)
    track_->setSolo(soloButton_.getToggleState());
}

//==============================================================================
// LevelMeter Implementation
//==============================================================================

void MixerChannelComponent::LevelMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(bounds);

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(bounds, 1);

    // Level bar
    float currentLevel = level_.load();

    if (currentLevel > 0.001f)
    {
        // Convert to dB for better visualization
        float levelDb = juce::Decibels::gainToDecibels(currentLevel);

        // Map -60dB to 0dB -> 0.0 to 1.0
        float normalizedLevel = juce::jmap(levelDb, -60.0f, 0.0f, 0.0f, 1.0f);
        normalizedLevel = juce::jlimit(0.0f, 1.0f, normalizedLevel);

        int barHeight = static_cast<int>(bounds.getHeight() * normalizedLevel);
        auto meterBounds = bounds.withTop(bounds.getBottom() - barHeight).reduced(2);

        // Color gradient based on level
        juce::Colour meterColor;
        if (normalizedLevel > 0.9f)
            meterColor = juce::Colours::red;      // Clipping warning
        else if (normalizedLevel > 0.7f)
            meterColor = juce::Colours::orange;   // Hot
        else if (normalizedLevel > 0.5f)
            meterColor = juce::Colours::yellow;   // Moderate
        else
            meterColor = juce::Colours::green;    // Normal

        g.setColour(meterColor);
        g.fillRect(meterBounds);
    }
}

void MixerChannelComponent::LevelMeter::setLevel(float level)
{
    level_.store(juce::jlimit(0.0f, 1.0f, level));
}
