/**
 * @file MixerView.cpp
 * @brief Mixer view implementation
 */

#include "MixerView.h"
#include "engine/ProjectState.h"
#include "MixerChannelComponent.h"
#include "Engine.h"
#include "../../Source/engine/Track.h"
#include "../../Source/engine/ProjectState.h"

using namespace zenith;
#include "../design-system/ZenithTheme.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithDesignSystem.h"

//==============================================================================
MixerView::MixerView(Engine& engine, ProjectState& state)
    : engine_(engine), state_(state)
{
    // Set up viewport for scrolling
    viewport_.setViewedComponent(&channelContainer_, false);
    viewport_.setScrollBarsShown(true, false);  // Vertical scrollbar, no horizontal
    addAndMakeVisible(viewport_);

    // Register as listener to ProjectState
    state_.getState().addListener(this);

    // Initial channel rebuild
    rebuildChannels();
}

MixerView::~MixerView()
{
    state_.getState().removeListener(this);
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
        g.setFont(design::typography::getJuceFont(16.0f));
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
//==============================================================================
void MixerView::valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& /*childWhichHasBeenAdded*/)
{
    if (parentTree.hasType(ProjectState::ID_TRACKS))
    {
        // Use Async call to ensure Engine has processed the change first (listener ordering)
        juce::MessageManager::callAsync([this]() { rebuildChannels(); });
    }
}

void MixerView::valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& /*childWhichHasBeenRemoved*/, int)
{
    if (parentTree.hasType(ProjectState::ID_TRACKS))
        juce::MessageManager::callAsync([this]() { rebuildChannels(); });
}

void MixerView::valueTreeChildOrderChanged(juce::ValueTree& parentTree, int, int)
{
    if (parentTree.hasType(ProjectState::ID_TRACKS))
        juce::MessageManager::callAsync([this]() { rebuildChannels(); });
}

void MixerView::valueTreeRedirected(juce::ValueTree& treeWhichHasBeenChanged)
{
    if (treeWhichHasBeenChanged == state_.getState())
        juce::MessageManager::callAsync([this]() { rebuildChannels(); });
}

void MixerView::rebuildChannels()
{
    DBG("MixerView: Rebuilding mixer channels from ProjectState");

    // Clear existing channels
    channels_.clear();
    channelContainer_.deleteAllChildren();

    // Get tracks from ProjectState (Source of Truth)
    auto tracksNode = state_.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid()) return;

    // Create a channel for each track in the project state
    for (const auto& trackNode : tracksNode)
    {
        juce::String trackId = trackNode[ProjectState::PROP_ID];
        
        // Lookup the corresponding engine track
        // This is safe because we use the ID as the key, not strict index
        auto* track = engine_.getTrackById(trackId);

        if (track != nullptr)
        {
            auto channel = std::make_unique<MixerChannelComponent>(track, state_, engine_);
            channelContainer_.addAndMakeVisible(channel.get());
            channels_.push_back(std::move(channel));
        }
        else
        {
            // This might happen if UI updates before EngineSync has finished.
            // In a real scenario, we might want to schedule a retry, but callAsync handles most cases.
            DBG("MixerView Warning: Track found in ProjectState but not yet in Engine: " + trackId);
        }
    }

    DBG("MixerView: Created " + juce::String(channels_.size()) + " mixer channels");

    // Update layout
    resized();
    repaint();
}

