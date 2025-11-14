/**
 * @file MixerComponent.cpp
 * @brief Mixer UI implementation
 */

#include "../include/MixerComponent.h"

//==============================================================================
// TrackStrip Implementation
//==============================================================================

TrackStrip::TrackStrip(ProjectState& projectState, Engine& engine, int trackIndex)
    : projectState_(projectState), engine_(engine), trackIndex_(trackIndex)
{
    // Get track ID from ProjectState
    auto track = projectState_.getTrackByIndex(trackIndex_);
    if (track.isValid())
    {
        trackId_ = track[ProjectState::PROP_ID].toString();
    }

    // Name label
    nameLabel_.setJustificationType(juce::Justification::centred);
    nameLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(nameLabel_);

    // Volume slider (vertical)
    volumeSlider_.setSliderStyle(juce::Slider::LinearVertical);
    volumeSlider_.setRange(0.0, 1.0, 0.01);
    volumeSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider_.onValueChange = [this]() { onVolumeChanged(); };
    addAndMakeVisible(volumeSlider_);

    // Pan slider (rotary)
    panSlider_.setSliderStyle(juce::Slider::Rotary);
    panSlider_.setRange(-1.0, 1.0, 0.01);
    panSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider_.onValueChange = [this]() { onPanChanged(); };
    addAndMakeVisible(panSlider_);

    // Mute button
    muteButton_.setButtonText("M");
    muteButton_.setClickingTogglesState(true);
    muteButton_.onClick = [this]() { onMuteToggled(); };
    addAndMakeVisible(muteButton_);

    // Solo button
    soloButton_.setButtonText("S");
    soloButton_.setClickingTogglesState(true);
    soloButton_.onClick = [this]() { onSoloToggled(); };
    addAndMakeVisible(soloButton_);

    // Arm button
    armButton_.setButtonText("R");
    armButton_.setClickingTogglesState(true);
    armButton_.onClick = [this]() { onArmToggled(); };
    addAndMakeVisible(armButton_);

    // Initial refresh
    refreshFromState();
}

void TrackStrip::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Border
    g.setColour(juce::Colour(0xff404040));
    g.drawRect(getLocalBounds(), 1);

    // Meter background (left of volume slider)
    auto bounds = getLocalBounds().reduced(4);
    auto meterArea = bounds.removeFromLeft(12).removeFromTop(bounds.getHeight() - 140);

    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(meterArea);

    // Meter level
    if (meterLevel_ > 0.0f)
    {
        const int meterHeight = meterArea.getHeight();
        const int levelHeight = static_cast<int>(meterLevel_ * meterHeight);

        auto levelRect = meterArea.removeFromBottom(levelHeight);

        // Color based on level
        juce::Colour meterColour;
        if (meterLevel_ > 0.95f)
            meterColour = juce::Colours::red;       // Clipping
        else if (meterLevel_ > 0.75f)
            meterColour = juce::Colours::orange;    // Hot
        else
            meterColour = juce::Colours::green;     // Normal

        g.setColour(meterColour);
        g.fillRect(levelRect);
    }
}

void TrackStrip::resized()
{
    auto bounds = getLocalBounds().reduced(4);

    // Name at top
    nameLabel_.setBounds(bounds.removeFromTop(20));

    // Buttons at bottom
    auto buttonArea = bounds.removeFromBottom(80);
    armButton_.setBounds(buttonArea.removeFromBottom(25).reduced(2));
    soloButton_.setBounds(buttonArea.removeFromBottom(25).reduced(2));
    muteButton_.setBounds(buttonArea.removeFromBottom(25).reduced(2));

    // Pan knob above buttons
    panSlider_.setBounds(bounds.removeFromBottom(40).reduced(4));

    // Reserve space for meter on left
    bounds.removeFromLeft(16);

    // Volume slider takes remaining space
    volumeSlider_.setBounds(bounds);
}

void TrackStrip::setMeterLevel(float level)
{
    meterLevel_ = juce::jlimit(0.0f, 1.2f, level);  // Allow slight overload display
    repaint();
}

void TrackStrip::refreshFromState()
{
    if (trackId_.isEmpty())
        return;

    // Update name
    nameLabel_.setText(projectState_.getTrackName(trackId_), juce::dontSendNotification);

    // Update controls without triggering callbacks
    volumeSlider_.setValue(projectState_.getTrackVolume(trackId_), juce::dontSendNotification);
    panSlider_.setValue(projectState_.getTrackPan(trackId_), juce::dontSendNotification);
    muteButton_.setToggleState(projectState_.isTrackMuted(trackId_), juce::dontSendNotification);
    soloButton_.setToggleState(projectState_.isTrackSolo(trackId_), juce::dontSendNotification);
    armButton_.setToggleState(projectState_.isTrackArmed(trackId_), juce::dontSendNotification);

    // Update button colors
    muteButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    soloButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow);
    armButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
}

void TrackStrip::onVolumeChanged()
{
    if (!trackId_.isEmpty())
    {
        projectState_.setTrackVolume(trackId_, static_cast<float>(volumeSlider_.getValue()));
    }
}

void TrackStrip::onPanChanged()
{
    if (!trackId_.isEmpty())
    {
        projectState_.setTrackPan(trackId_, static_cast<float>(panSlider_.getValue()));
    }
}

void TrackStrip::onMuteToggled()
{
    if (!trackId_.isEmpty())
    {
        projectState_.setTrackMute(trackId_, muteButton_.getToggleState());
    }
}

void TrackStrip::onSoloToggled()
{
    if (!trackId_.isEmpty())
    {
        projectState_.setTrackSolo(trackId_, soloButton_.getToggleState());
    }
}

void TrackStrip::onArmToggled()
{
    if (!trackId_.isEmpty())
    {
        projectState_.setTrackArmed(trackId_, armButton_.getToggleState());
    }
}

//==============================================================================
// MixerComponent Implementation
//==============================================================================

MixerComponent::MixerComponent(ProjectState& projectState, Engine& engine)
    : projectState_(projectState), engine_(engine)
{
    // Master label
    masterLabel_.setText("Master", juce::dontSendNotification);
    masterLabel_.setJustificationType(juce::Justification::centredTop);
    masterLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(masterLabel_);

    // Build initial track strips
    rebuildTracks();

    // Start timer for meter updates (30 Hz)
    startTimer(33);
}

MixerComponent::~MixerComponent()
{
    stopTimer();
}

void MixerComponent::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Master meter (simple for now)
    auto bounds = getLocalBounds();
    auto masterArea = bounds.removeFromRight(MASTER_WIDTH).reduced(4);

    // Master meter background
    auto meterArea = masterArea.removeFromLeft(16).removeFromTop(masterArea.getHeight() - 24);

    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(meterArea);

    // Master meter level
    if (masterMeterLevel_ > 0.0f)
    {
        const int meterHeight = meterArea.getHeight();
        const int levelHeight = static_cast<int>(masterMeterLevel_ * meterHeight);

        auto levelRect = meterArea.removeFromBottom(levelHeight);

        // Color based on level
        juce::Colour meterColour;
        if (masterMeterLevel_ > 0.95f)
            meterColour = juce::Colours::red;
        else if (masterMeterLevel_ > 0.75f)
            meterColour = juce::Colours::orange;
        else
            meterColour = juce::Colours::green;

        g.setColour(meterColour);
        g.fillRect(levelRect);
    }
}

void MixerComponent::resized()
{
    auto bounds = getLocalBounds();

    // Master section on right
    auto masterArea = bounds.removeFromRight(MASTER_WIDTH);
    masterLabel_.setBounds(masterArea.removeFromTop(20).reduced(4));

    // Track strips from left to right
    int x = 0;
    for (auto* strip : trackStrips_)
    {
        strip->setBounds(x, 0, STRIP_WIDTH, getHeight());
        x += STRIP_WIDTH;
    }
}

void MixerComponent::rebuildTracks()
{
    // Clear existing strips
    trackStrips_.clear();

    // Create strip for each track
    const int numTracks = projectState_.getNumTracks();

    for (int i = 0; i < numTracks; ++i)
    {
        auto* strip = trackStrips_.add(new TrackStrip(projectState_, engine_, i));
        addAndMakeVisible(strip);
    }

    // Request layout
    resized();
}

void MixerComponent::timerCallback()
{
    // Update track meters from engine
    for (auto* strip : trackStrips_)
    {
        const int trackIndex = strip->getTrackIndex();
        const float level = engine_.getTrackLevel(trackIndex);
        strip->setMeterLevel(level);
    }

    // Update master meter
    masterMeterLevel_ = engine_.getMasterLevel();
    repaint();  // Redraw master meter
}
