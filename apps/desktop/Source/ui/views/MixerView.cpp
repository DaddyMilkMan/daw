/**
 * @file MixerView.cpp
 * @brief Mixer view implementation
 */

#include "MixerView.h"
#include "MixerChannelComponent.h"
#include "Engine.h"
#include "../../Source/engine/Track.h"

using namespace zenith;
#include "../design-system/ZenithTheme.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithTypography.h"

//==============================================================================
MixerView::MixerView(Engine& engine, ProjectState& state)
    : engine_(engine), state_(state)
{
    // Set up viewport for scrolling
    viewport_.setViewedComponent(&channelContainer_, false);
    viewport_.setScrollBarsShown(true, false);  // Vertical scrollbar, no horizontal
    addAndMakeVisible(viewport_);

    // Initial channel rebuild
    rebuildChannels();

    // Start timer to check for track count changes (10 Hz)
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimer(100);
}

MixerView::~MixerView()
{
    stopTimer();
    channels_.clear();
}

//==============================================================================
void MixerView::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(design::toJuceColour(design::unified::bg_00()));

    // If no tracks, show helpful message
    if (channels_.empty())
    {
        g.setColour(design::toJuceColour(design::unified::text_tertiary()));
        g.setFont(ZenithTypography::getBodyFont().withHeight(16.0f));
        g.drawText("No tracks in mixer",
                   getLocalBounds(),
                   juce::Justification::centred,
                   true);
    }
}

void MixerView::resized()
{
    // Viewport fills entire component
    viewport_.setBounds(getLocalBounds());

    // Layout channels horizontally
    const int channelWidth = 80;
    const int channelHeight = 400;
    const int spacing = 5;

    int totalWidth = 0;
    int xPos = spacing;

    for (auto& channel : channels_)
    {
        if (channel != nullptr)
        {
            channel->setBounds(xPos, spacing, channelWidth, channelHeight);
            xPos += channelWidth + spacing;
            totalWidth = xPos;
        }
    }

    // Set container size to fit all channels
    channelContainer_.setSize(juce::jmax(totalWidth, getWidth()), channelHeight + spacing * 2);
}

//==============================================================================
void MixerView::timerCallback()
{
    // Check if track count has changed
    int currentTrackCount = engine_.getNumTracks();

    if (currentTrackCount != lastTrackCount_)
    {
        DBG("MixerView: Track count changed from " + juce::String(lastTrackCount_)
            + " to " + juce::String(currentTrackCount) + ", rebuilding channels");
        rebuildChannels();
        lastTrackCount_ = currentTrackCount;
    }
}

void MixerView::rebuildChannels()
{
    DBG("MixerView: Rebuilding mixer channels");

    // Clear existing channels
    channels_.clear();
    channelContainer_.deleteAllChildren();

    // Get tracks from engine (message thread only)
    const auto& tracks = engine_.tracks();

    // Create a channel for each track
    for (const auto& track : tracks)
    {
        if (track != nullptr)
        {
            auto channel = std::make_unique<MixerChannelComponent>(track.get(), state_, engine_);
            channelContainer_.addAndMakeVisible(channel.get());
            channels_.push_back(std::move(channel));
        }
    }

    DBG("MixerView: Created " + juce::String(channels_.size()) + " mixer channels");

    // Update layout
    resized();
    repaint();
}

