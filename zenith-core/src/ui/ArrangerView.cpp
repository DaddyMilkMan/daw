/**
 * @file ArrangerView.cpp
 * @brief Arranger view implementation
 */

#include "../../include/ui/ArrangerView.h"

//==============================================================================
ArrangerView::ArrangerView(ProjectState& projectState)
    : projectState_(projectState)
{
    // Listen to TRACKS node for track add/remove
    auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (tracksNode.isValid())
        tracksNode.addListener(this);

    // Build initial track headers
    rebuildTrackHeaders();
}

ArrangerView::~ArrangerView()
{
    // Remove listener
    auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (tracksNode.isValid())
        tracksNode.removeListener(this);
}

//==============================================================================
void ArrangerView::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Draw background
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Draw header area background
    auto headerArea = bounds.removeFromLeft(HEADER_WIDTH);
    g.setColour(juce::Colour(0xff252525));
    g.fillRect(headerArea);

    // Draw separator line between headers and timeline
    g.setColour(juce::Colour(0xff0a0a0a));
    g.drawVerticalLine(HEADER_WIDTH, 0.0f, static_cast<float>(getHeight()));

    // Timeline area (placeholder)
    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillRect(bounds);

    // Draw placeholder text
    if (trackHeaders_.empty())
    {
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(16.0f));
        g.drawText("No tracks. Use Project > Add Track to create tracks.",
                   getLocalBounds(),
                   juce::Justification::centred,
                   true);
    }
    else
    {
        // Draw timeline placeholder text
        g.setColour(juce::Colours::darkgrey);
        g.setFont(juce::Font(14.0f));
        auto timelineArea = getLocalBounds().removeFromLeft(getWidth()).removeFromLeft(getWidth() - HEADER_WIDTH);
        g.drawText("Timeline view (coming soon)",
                   timelineArea,
                   juce::Justification::centred,
                   true);
    }
}

void ArrangerView::resized()
{
    layoutTrackHeaders();
}

//==============================================================================
// ValueTree::Listener (MESSAGE THREAD)
//==============================================================================

void ArrangerView::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    // Check if a track was added to TRACKS node
    if (parent.hasType(ProjectState::ID_TRACKS) && child.hasType(ProjectState::ID_TRACK))
    {
        DBG("ArrangerView: Track added, rebuilding headers");
        rebuildTrackHeaders();
    }
}

void ArrangerView::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    juce::ignoreUnused(index);

    // Check if a track was removed from TRACKS node
    if (parent.hasType(ProjectState::ID_TRACKS) && child.hasType(ProjectState::ID_TRACK))
    {
        DBG("ArrangerView: Track removed, rebuilding headers");
        rebuildTrackHeaders();
    }
}

void ArrangerView::valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex)
{
    juce::ignoreUnused(oldIndex, newIndex);

    // Check if track order changed in TRACKS node
    if (parent.hasType(ProjectState::ID_TRACKS))
    {
        DBG("ArrangerView: Track order changed, rebuilding headers");
        rebuildTrackHeaders();
    }
}

//==============================================================================
// Helper Methods
//==============================================================================

void ArrangerView::rebuildTrackHeaders()
{
    // Clear existing headers
    trackHeaders_.clear();

    // Get TRACKS node
    auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    // Create header for each track
    for (auto trackNode : tracksNode)
    {
        if (!trackNode.hasType(ProjectState::ID_TRACK))
            continue;

        juce::String trackId = trackNode[ProjectState::PROP_ID].toString();

        auto header = std::make_unique<TrackHeaderComponent>(projectState_, trackId);
        addAndMakeVisible(*header);
        trackHeaders_.push_back(std::move(header));
    }

    // Relayout
    layoutTrackHeaders();
    repaint();

    DBG("ArrangerView: Rebuilt " + juce::String(trackHeaders_.size()) + " track headers");
}

void ArrangerView::layoutTrackHeaders()
{
    int yPos = 0;

    for (auto& header : trackHeaders_)
    {
        header->setBounds(0, yPos, HEADER_WIDTH, TRACK_HEIGHT);
        yPos += TRACK_HEIGHT;
    }
}
