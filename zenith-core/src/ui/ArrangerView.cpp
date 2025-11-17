/**
 * @file ArrangerView.cpp
 * @brief Arranger view implementation
 */

#include "../../include/ui/ArrangerView.h"

ArrangerView::ArrangerView(ProjectState& ps)
    : projectState(ps)
{
    // Listen to project state changes
    projectState.getState().addListener(this);

    // Add ruler
    addAndMakeVisible(ruler);
    ruler.setVisibleRange(viewStartBeat, viewLengthBeats);

    // Build initial clip components
    rebuildClips();
}

ArrangerView::~ArrangerView()
{
    projectState.getState().removeListener(this);
}

void ArrangerView::setZoom(double newPixelsPerBeat)
{
    pixelsPerBeat = juce::jlimit(5.0, 100.0, newPixelsPerBeat);
    viewLengthBeats = getWidth() / pixelsPerBeat;
    ruler.setVisibleRange(viewStartBeat, viewLengthBeats);
    updateClipBounds();
}

void ArrangerView::scrollToBeat(double beat)
{
    viewStartBeat = juce::jmax(0.0, beat);
    ruler.setVisibleRange(viewStartBeat, viewLengthBeats);
    updateClipBounds();
}

void ArrangerView::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(RULER_HEIGHT);

    // Background
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Track lanes
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    int numTracks = tracksNode.getNumChildren();

    for (int i = 0; i < numTracks; ++i)
    {
        int y = RULER_HEIGHT + i * TRACK_HEIGHT;

        // Alternating track colors
        if (i % 2 == 0)
            g.setColour(juce::Colour(0xff252525));
        else
            g.setColour(juce::Colour(0xff2a2a2a));

        g.fillRect(0, y, getWidth(), TRACK_HEIGHT);

        // Track separator
        g.setColour(juce::Colour(0xff404040));
        g.drawLine(0, y + TRACK_HEIGHT - 1.0f, static_cast<float>(getWidth()),
                   y + TRACK_HEIGHT - 1.0f, 1.0f);

        // Track name
        auto track = tracksNode.getChild(i);
        juce::String trackName = track[ProjectState::PROP_NAME].toString();
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(14.0f));
        g.drawText(trackName, 10, y + 10, 200, 20, juce::Justification::centredLeft);
    }

    // Beat grid
    g.setColour(juce::Colour(0xff303030));
    int startBeat = static_cast<int>(std::floor(viewStartBeat));
    int endBeat = static_cast<int>(std::ceil(viewStartBeat + viewLengthBeats));

    for (int beat = startBeat; beat <= endBeat; ++beat)
    {
        int x = static_cast<int>((beat - viewStartBeat) * pixelsPerBeat);

        if (x < 0 || x > getWidth())
            continue;

        // Measure lines (every 4 beats)
        if (beat % 4 == 0)
        {
            g.setColour(juce::Colour(0xff404040));
            g.drawLine(static_cast<float>(x), RULER_HEIGHT,
                      static_cast<float>(x), static_cast<float>(getHeight()), 1.0f);
        }
    }
}

void ArrangerView::resized()
{
    // Position ruler
    ruler.setBounds(0, 0, getWidth(), RULER_HEIGHT);

    // Update view length based on width
    if (pixelsPerBeat > 0)
        viewLengthBeats = getWidth() / pixelsPerBeat;

    ruler.setVisibleRange(viewStartBeat, viewLengthBeats);

    // Update clip bounds
    updateClipBounds();
}

void ArrangerView::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
        return;

    int trackIdx = getTrackAtY(event.y);
    double beat = getBeatAtX(event.x);

    DBG("Clicked track " + juce::String(trackIdx) + " at beat " + juce::String(beat));
}

void ArrangerView::mouseDoubleClick(const juce::MouseEvent& event)
{
    // Find clip at position
    for (auto& clipComp : clipComponents)
    {
        if (clipComp->getBounds().contains(event.getPosition()))
        {
            DBG("Double-clicked clip: " + clipComp->getClipId());
            // TODO: Open piano roll for MIDI clips
            return;
        }
    }

    // Double-click on empty space: create a clip
    int trackIdx = getTrackAtY(event.y);
    double beat = getBeatAtX(event.x);

    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid() || trackIdx < 0 || trackIdx >= tracksNode.getNumChildren())
        return;

    auto track = tracksNode.getChild(trackIdx);
    juce::String trackId = track[ProjectState::PROP_ID].toString();

    // Snap to beat
    double snappedBeat = std::round(beat);

    // Create a 4-beat clip
    projectState.addClip(trackId, snappedBeat, 4.0, "Add Clip");
    DBG("Created clip at beat " + juce::String(snappedBeat) + " on track " + trackId);
}

//==========================================================================
// ValueTree::Listener implementation
//==========================================================================

void ArrangerView::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    // Clip moved/resized
    if (tree.hasType(ProjectState::ID_CLIP) &&
        (property == ProjectState::PROP_START_BEATS || property == ProjectState::PROP_LENGTH_BEATS))
    {
        updateClipBounds();
    }
}

void ArrangerView::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    // New clip or track added
    if (child.hasType(ProjectState::ID_CLIP) || child.hasType(ProjectState::ID_TRACK))
    {
        rebuildClips();
    }
}

void ArrangerView::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    // Clip or track removed
    if (child.hasType(ProjectState::ID_CLIP) || child.hasType(ProjectState::ID_TRACK))
    {
        rebuildClips();
    }
}

//==========================================================================
// Helper methods
//==========================================================================

void ArrangerView::rebuildClips()
{
    // Clear existing clip components
    clipComponents.clear();

    // Iterate through all tracks and clips
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    for (int trackIdx = 0; trackIdx < tracksNode.getNumChildren(); ++trackIdx)
    {
        auto track = tracksNode.getChild(trackIdx);
        auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);

        if (!clipsNode.isValid())
            continue;

        for (int clipIdx = 0; clipIdx < clipsNode.getNumChildren(); ++clipIdx)
        {
            auto clip = clipsNode.getChild(clipIdx);
            auto clipComp = std::make_unique<ClipComponent>(clip);
            addAndMakeVisible(*clipComp);
            clipComponents.push_back(std::move(clipComp));
        }
    }

    updateClipBounds();
}

void ArrangerView::updateClipBounds()
{
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    for (auto& clipComp : clipComponents)
    {
        // Find which track this clip belongs to
        auto clipNode = clipComp->getClipNode();
        auto clipsNode = clipNode.getParent();
        auto trackNode = clipsNode.getParent();

        // Find track index
        int trackIdx = tracksNode.indexOf(trackNode);
        if (trackIdx < 0)
            continue;

        // Calculate position
        int y = RULER_HEIGHT + trackIdx * TRACK_HEIGHT;
        int height = TRACK_HEIGHT - 4; // Small margin

        // Update clip bounds (accounting for view scroll)
        double startBeats = clipComp->getStartBeats() - viewStartBeat;
        clipComp->updateBounds(pixelsPerBeat, y + 2, height);

        // Adjust for scroll
        int x = static_cast<int>(startBeats * pixelsPerBeat);
        int width = static_cast<int>(clipComp->getLengthBeats() * pixelsPerBeat);
        clipComp->setBounds(x, y + 2, width, height);
    }

    repaint();
}

int ArrangerView::getTrackAtY(int y) const
{
    if (y < RULER_HEIGHT)
        return -1;

    int trackY = y - RULER_HEIGHT;
    return trackY / TRACK_HEIGHT;
}

double ArrangerView::getBeatAtX(int x) const
{
    return viewStartBeat + (x / pixelsPerBeat);
}
