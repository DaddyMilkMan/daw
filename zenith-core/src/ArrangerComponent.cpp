/**
 * @file ArrangerComponent.cpp
 * @brief Arranger implementation
 */

#include "../include/ArrangerComponent.h"

//==============================================================================
// TrackHeader Implementation
//==============================================================================

ArrangerComponent::TrackHeader::TrackHeader(ProjectState& ps, const juce::String& id)
    : projectState(ps), trackId(id)
{
    // Track name
    nameLabel.setText(trackId, juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(nameLabel);

    // Track type badge
    auto trackType = projectState.getTrackType(trackId);
    typeLabel.setText(trackType == "audio" ? "A" : "M", juce::dontSendNotification);
    typeLabel.setJustificationType(juce::Justification::centred);
    typeLabel.setColour(juce::Label::backgroundColourId, trackType == "audio" ? juce::Colours::blue : juce::Colours::green);
    typeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(typeLabel);

    // Arm button
    armButton.setButtonText("R");
    armButton.setClickingTogglesState(true);
    armButton.setToggleState(projectState.isTrackArmed(trackId), juce::dontSendNotification);
    armButton.onClick = [this]() {
        projectState.setTrackArmed(trackId, armButton.getToggleState());
    };
    addAndMakeVisible(armButton);
}

void ArrangerComponent::TrackHeader::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds());
}

void ArrangerComponent::TrackHeader::resized()
{
    auto bounds = getLocalBounds().reduced(4);

    // Type badge on left
    typeLabel.setBounds(bounds.removeFromLeft(30));

    // Arm button on right
    armButton.setBounds(bounds.removeFromRight(30));

    // Name in middle
    nameLabel.setBounds(bounds.reduced(4, 0));
}

//==============================================================================
// ArrangerComponent Implementation
//==============================================================================

ArrangerComponent::ArrangerComponent(ProjectState& ps)
    : projectState_(ps)
{
    // Listen to project state changes
    projectState_.getState().addListener(this);

    // Build initial track components
    rebuildTrackComponents();
}

ArrangerComponent::~ArrangerComponent()
{
    projectState_.getState().removeListener(this);
}

void ArrangerComponent::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Draw timeline area
    auto timelineArea = getLocalBounds().withTrimmedLeft(headerWidth);
    g.setColour(juce::Colour(0xff252525));
    g.fillRect(timelineArea);

    // Draw track separators
    g.setColour(juce::Colours::darkgrey);
    for (int i = 0; i <= trackHeaders_.size(); ++i)
    {
        int y = i * trackHeight;
        g.drawLine(0, static_cast<float>(y), static_cast<float>(getWidth()), static_cast<float>(y));
    }

    // Draw header separator
    g.drawLine(static_cast<float>(headerWidth), 0, static_cast<float>(headerWidth), static_cast<float>(getHeight()));

    // Draw clips
    auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (tracksNode.isValid())
    {
        int trackIndex = 0;
        for (auto track : tracksNode)
        {
            auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);
            if (clipsNode.isValid())
            {
                for (auto clip : clipsNode)
                {
                    auto startSamples = static_cast<int64_t>(clip[ProjectState::PROP_START]);
                    auto lengthSamples = static_cast<int64_t>(clip[ProjectState::PROP_LENGTH]);
                    auto clipType = clip[ProjectState::PROP_TYPE].toString();

                    auto clipBounds = getClipBounds(trackIndex, startSamples, lengthSamples);

                    // Draw clip
                    if (clipType == "audio")
                    {
                        g.setColour(juce::Colours::blue.withAlpha(0.7f));
                    }
                    else
                    {
                        g.setColour(juce::Colours::green.withAlpha(0.7f));
                    }

                    g.fillRoundedRectangle(clipBounds.toFloat(), 4.0f);

                    g.setColour(juce::Colours::white.withAlpha(0.9f));
                    g.drawRoundedRectangle(clipBounds.toFloat(), 4.0f, 1.0f);

                    // Draw clip name
                    g.setColour(juce::Colours::white);
                    g.setFont(12.0f);
                    g.drawText(clip[ProjectState::PROP_ID].toString(),
                              clipBounds.reduced(4),
                              juce::Justification::centredLeft,
                              true);
                }
            }

            trackIndex++;
        }
    }
}

void ArrangerComponent::resized()
{
    // Position track headers
    for (int i = 0; i < trackHeaders_.size(); ++i)
    {
        auto bounds = getTrackBounds(i).withWidth(headerWidth);
        trackHeaders_[i]->setBounds(bounds);
    }
}

//==============================================================================
// ValueTree::Listener
//==============================================================================

void ArrangerComponent::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    juce::ignoreUnused(tree, property);
    repaint();
}

void ArrangerComponent::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    juce::ignoreUnused(parent, child);

    // Rebuild track components if tracks were added
    if (parent.getType() == ProjectState::ID_TRACKS)
    {
        rebuildTrackComponents();
    }

    repaint();
}

void ArrangerComponent::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    juce::ignoreUnused(parent, child, index);

    // Rebuild track components if tracks were removed
    if (parent.getType() == ProjectState::ID_TRACKS)
    {
        rebuildTrackComponents();
    }

    repaint();
}

//==============================================================================
// Helper methods
//==============================================================================

void ArrangerComponent::rebuildTrackComponents()
{
    trackHeaders_.clear();

    auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (tracksNode.isValid())
    {
        for (auto track : tracksNode)
        {
            auto trackId = track[ProjectState::PROP_ID].toString();
            auto header = new TrackHeader(projectState_, trackId);
            trackHeaders_.add(header);
            addAndMakeVisible(header);
        }
    }

    resized();
}

juce::Rectangle<int> ArrangerComponent::getTrackBounds(int trackIndex) const
{
    return juce::Rectangle<int>(0, trackIndex * trackHeight, getWidth(), trackHeight);
}

juce::Rectangle<int> ArrangerComponent::getClipBounds(int trackIndex, int64_t startSamples, int64_t lengthSamples) const
{
    // Convert samples to pixels (simplified - assumes 44100 Hz)
    const double sampleRate = 44100.0;
    int startX = headerWidth + static_cast<int>((startSamples / sampleRate) * pixelsPerSecond);
    int width = static_cast<int>((lengthSamples / sampleRate) * pixelsPerSecond);

    auto trackBounds = getTrackBounds(trackIndex);

    return juce::Rectangle<int>(startX, trackBounds.getY() + 4, width, trackHeight - 8);
}
