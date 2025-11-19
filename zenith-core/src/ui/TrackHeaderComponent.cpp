/**
 * @file TrackHeaderComponent.cpp
 * @brief Track header implementation
 */

#include "../../include/ui/TrackHeaderComponent.h"

//==============================================================================
TrackHeaderComponent::TrackHeaderComponent(ProjectState& projectState, const juce::String& trackId)
    : projectState_(projectState), trackId_(trackId)
{
    // Get track node from ProjectState
    trackNode_ = projectState_.getTrack(trackId_);
    jassert(trackNode_.isValid());

    // Listen to track node changes
    if (trackNode_.isValid())
        trackNode_.addListener(this);

    // Setup name label (editable)
    nameLabel_.setEditable(true);
    nameLabel_.setJustificationType(juce::Justification::centredLeft);
    nameLabel_.setFont(juce::Font(14.0f));
    nameLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    nameLabel_.setColour(juce::Label::backgroundColourId, juce::Colour(0xff2a2a2a));
    nameLabel_.onTextChange = [this]() { onNameChanged(); };
    addAndMakeVisible(nameLabel_);

    // Setup Mute button
    muteButton_.setButtonText("M");
    muteButton_.setClickingTogglesState(true);
    muteButton_.onClick = [this]() { onMuteClicked(); };
    addAndMakeVisible(muteButton_);

    // Setup Solo button
    soloButton_.setButtonText("S");
    soloButton_.setClickingTogglesState(true);
    soloButton_.onClick = [this]() { onSoloClicked(); };
    addAndMakeVisible(soloButton_);

    // Setup Arm button
    armButton_.setButtonText("R");
    armButton_.setClickingTogglesState(true);
    armButton_.onClick = [this]() { onArmClicked(); };
    addAndMakeVisible(armButton_);

    // Load initial state
    updateFromState();
}

TrackHeaderComponent::~TrackHeaderComponent()
{
    if (trackNode_.isValid())
        trackNode_.removeListener(this);
}

//==============================================================================
void TrackHeaderComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Draw background
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(bounds);

    // Draw color stripe (left edge, 8px wide)
    auto colorStripe = bounds.removeFromLeft(8);
    g.setColour(trackColour_);
    g.fillRect(colorStripe);

    // Draw border
    g.setColour(juce::Colour(0xff1a1a1a));
    g.drawRect(bounds, 1);
}

void TrackHeaderComponent::resized()
{
    auto bounds = getLocalBounds();

    // Remove color stripe area
    bounds.removeFromLeft(8);

    // Add padding
    bounds.reduce(4, 4);

    // Buttons on the right (24x24 each)
    const int buttonWidth = 24;
    const int buttonHeight = 24;
    const int spacing = 4;

    auto buttonArea = bounds.removeFromRight(buttonWidth * 3 + spacing * 2);
    buttonArea = buttonArea.withSizeKeepingCentre(buttonWidth * 3 + spacing * 2, buttonHeight);

    muteButton_.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(spacing);
    soloButton_.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(spacing);
    armButton_.setBounds(buttonArea.removeFromLeft(buttonWidth));

    // Name label takes remaining space
    bounds.removeFromRight(spacing);  // Spacing between name and buttons
    nameLabel_.setBounds(bounds);
}

//==============================================================================
// ValueTree::Listener (MESSAGE THREAD)
//==============================================================================

void TrackHeaderComponent::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree != trackNode_)
        return;

    // Update UI when properties change
    updateFromState();
}

//==============================================================================
// UI Callbacks
//==============================================================================

void TrackHeaderComponent::onNameChanged()
{
    auto newName = nameLabel_.getText();
    projectState_.setTrackName(trackId_, newName, "Change Track Name");
}

void TrackHeaderComponent::onMuteClicked()
{
    bool newMuted = muteButton_.getToggleState();
    projectState_.setTrackMute(trackId_, newMuted, "Toggle Mute");
}

void TrackHeaderComponent::onSoloClicked()
{
    bool newSoloed = soloButton_.getToggleState();
    projectState_.setTrackSolo(trackId_, newSoloed, "Toggle Solo");
}

void TrackHeaderComponent::onArmClicked()
{
    bool newArmed = armButton_.getToggleState();
    projectState_.setTrackArmed(trackId_, newArmed, "Toggle Record Arm");
}

//==============================================================================
// Helper Methods
//==============================================================================

void TrackHeaderComponent::updateFromState()
{
    if (!trackNode_.isValid())
        return;

    // Update name
    juce::String name = trackNode_[ProjectState::PROP_NAME].toString();
    if (nameLabel_.getText() != name)
        nameLabel_.setText(name, juce::dontSendNotification);

    // Update color
    int colourInt = trackNode_[ProjectState::PROP_COLOUR];
    trackColour_ = juce::Colour(static_cast<juce::uint32>(colourInt));

    // Update button states
    isMuted_ = trackNode_[ProjectState::PROP_MUTE];
    isSoloed_ = trackNode_[ProjectState::PROP_SOLO];
    isArmed_ = trackNode_[ProjectState::PROP_ARMED];

    muteButton_.setToggleState(isMuted_, juce::dontSendNotification);
    soloButton_.setToggleState(isSoloed_, juce::dontSendNotification);
    armButton_.setToggleState(isArmed_, juce::dontSendNotification);

    // Visual feedback
    // Muted: grey out
    if (isMuted_)
    {
        nameLabel_.setColour(juce::Label::textColourId, juce::Colours::grey);
        muteButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff666666));
    }
    else
    {
        nameLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
        muteButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    }

    // Soloed: highlight S button
    if (isSoloed_)
        soloButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffffaa00));
    else
        soloButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);

    // Armed: highlight R button (red)
    if (isArmed_)
        armButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff0000));
    else
        armButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);

    repaint();
}
