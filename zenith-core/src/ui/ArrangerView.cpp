/**
 * @file ArrangerView.cpp
 * @brief Arranger view implementation
 */

#include "../../include/ui/ArrangerView.h"

//==============================================================================
// TrackLane Implementation
//==============================================================================

TrackLane::TrackLane(ProjectState& state, const juce::String& tId)
    : projectState(state)
    , trackId(tId)
{
    // Get track info
    auto track = projectState.getTrackById(trackId);
    if (track.isValid())
        trackName = track[ProjectState::PROP_NAME].toString();

    // Listen to clips changes
    auto clipsNode = projectState.getClips(trackId);
    if (clipsNode.isValid())
        clipsNode.addListener(this);

    refreshClips();
}

TrackLane::~TrackLane()
{
    auto clipsNode = projectState.getClips(trackId);
    if (clipsNode.isValid())
        clipsNode.removeListener(this);
}

void TrackLane::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background (alternating colors for tracks)
    static int trackCounter = 0;
    bool isEven = (trackCounter++ % 2 == 0);
    g.fillAll(isEven ? juce::Colour(0xff1e1e1e) : juce::Colour(0xff252525));

    // Track name on the left
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText(trackName, 10, 10, 150, 20, juce::Justification::centredLeft);

    // Bottom border
    g.setColour(juce::Colour(0xff1a1a1a));
    g.drawLine(0.0f, (float)bounds.getBottom() - 1.0f,
               (float)bounds.getRight(), (float)bounds.getBottom() - 1.0f, 1.0f);
}

void TrackLane::resized()
{
    // Position clip components
    for (auto* clip : clipComponents)
    {
        double startBeats = clip->getStartBeats();
        double lengthBeats = clip->getLengthBeats();

        int x = (int)(startBeats * pixelsPerBeat) - scrollOffset;
        int width = (int)(lengthBeats * pixelsPerBeat);

        // Clip vertical position (leave space for track name)
        clip->setBounds(x, 30, width, getHeight() - 40);
    }
}

void TrackLane::setPixelsPerBeat(double ppb)
{
    pixelsPerBeat = ppb;
    resized();
    repaint();
}

void TrackLane::setScrollOffset(int offset)
{
    scrollOffset = offset;
    resized();
    repaint();
}

void TrackLane::refreshClips()
{
    // Clear existing clip components
    clipComponents.clear();

    // Get clips from ProjectState
    auto clipsNode = projectState.getClips(trackId);
    if (!clipsNode.isValid())
        return;

    // Create ClipComponent for each clip
    for (auto clip : clipsNode)
    {
        if (!clip.hasType(ProjectState::ID_CLIP))
            continue;

        juce::String clipId = clip[ProjectState::PROP_ID].toString();
        double startBeats = clip[ProjectState::PROP_START];
        double lengthBeats = clip[ProjectState::PROP_LENGTH];

        auto* clipComp = new ClipComponent(clipId, trackId, startBeats, lengthBeats);

        // Set up move callback
        clipComp->onClipMoved = [this](const juce::String& clipId, double newStartBeats)
        {
            onClipMoved(clipId, newStartBeats);
        };

        // Set pixels per beat for proper dragging
        // (ClipComponent uses this internally for drag calculations)

        clipComponents.add(clipComp);
        addAndMakeVisible(clipComp);
    }

    resized();
}

void TrackLane::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (child.hasType(ProjectState::ID_CLIP))
        refreshClips();
}

void TrackLane::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    if (child.hasType(ProjectState::ID_CLIP))
        refreshClips();
}

void TrackLane::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree.hasType(ProjectState::ID_CLIP) && property == ProjectState::PROP_START)
    {
        // Clip moved - update visual position
        juce::String clipId = tree[ProjectState::PROP_ID].toString();
        double newStart = tree[ProjectState::PROP_START];

        for (auto* clip : clipComponents)
        {
            if (clip->getClipId() == clipId)
            {
                clip->setStartBeats(newStart);
                resized();
                break;
            }
        }
    }
}

void TrackLane::onClipMoved(const juce::String& clipId, double newStartBeats)
{
    // Update ProjectState (which will trigger valueTreePropertyChanged)
    projectState.moveClip(trackId, clipId, newStartBeats, "Move Clip");
}

//==============================================================================
// ArrangerView Implementation
//==============================================================================

ArrangerView::ArrangerView(ProjectState& state)
    : projectState(state)
    , horizontalScrollBar(false)  // Horizontal
    , verticalScrollBar(true)     // Vertical
{
    // Create timeline ruler
    timelineRuler = std::make_unique<TimelineRuler>();
    addAndMakeVisible(timelineRuler.get());

    // Set time signature from project
    timelineRuler->setTimeSignature(projectState.getTimeSignatureNumerator(),
                                    projectState.getTimeSignatureDenominator());

    // Add scrollbars
    addAndMakeVisible(horizontalScrollBar);
    addAndMakeVisible(verticalScrollBar);

    horizontalScrollBar.addListener(this);
    verticalScrollBar.addListener(this);

    // Listen to tracks changes
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (tracksNode.isValid())
        tracksNode.addListener(this);

    refreshTracks();
    updateScrollBars();
}

ArrangerView::~ArrangerView()
{
    horizontalScrollBar.removeListener(this);
    verticalScrollBar.removeListener(this);

    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (tracksNode.isValid())
        tracksNode.removeListener(this);
}

//==============================================================================
// Component interface
//==============================================================================

void ArrangerView::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));

    // If no tracks, show message
    if (trackLanes.isEmpty())
    {
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(16.0f));
        g.drawText("No tracks - Add a track to get started",
                   bounds,
                   juce::Justification::centred,
                   true);
    }
}

void ArrangerView::resized()
{
    auto bounds = getLocalBounds();

    // Timeline ruler at top
    auto rulerBounds = bounds.removeFromTop(RULER_HEIGHT);
    timelineRuler->setBounds(rulerBounds);

    // Scrollbars at bottom and right
    auto horizontalScrollBounds = bounds.removeFromBottom(SCROLLBAR_SIZE);
    auto verticalScrollBounds = bounds.removeFromRight(SCROLLBAR_SIZE);

    // Leave space for scrollbar corner
    horizontalScrollBounds.removeFromRight(SCROLLBAR_SIZE);

    horizontalScrollBar.setBounds(horizontalScrollBounds);
    verticalScrollBar.setBounds(verticalScrollBounds);

    // Track lanes
    int totalHeight = trackLanes.size() * TRACK_HEIGHT;
    int y = -scrollOffsetY;

    for (auto* lane : trackLanes)
    {
        lane->setBounds(0, y, bounds.getWidth(), TRACK_HEIGHT);
        y += TRACK_HEIGHT;
    }

    updateScrollBars();
}

//==============================================================================
// Zoom control
//==============================================================================

void ArrangerView::setPixelsPerBeat(double ppb)
{
    pixelsPerBeat = juce::jlimit(10.0, 200.0, ppb);

    // Update timeline ruler
    timelineRuler->setPixelsPerBeat(pixelsPerBeat);

    // Update all track lanes
    for (auto* lane : trackLanes)
        lane->setPixelsPerBeat(pixelsPerBeat);

    updateScrollBars();
    repaint();
}

void ArrangerView::zoomIn()
{
    setPixelsPerBeat(pixelsPerBeat * 1.2);
}

void ArrangerView::zoomOut()
{
    setPixelsPerBeat(pixelsPerBeat / 1.2);
}

//==============================================================================
// ValueTree::Listener interface
//==============================================================================

void ArrangerView::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (child.hasType(ProjectState::ID_TRACK))
        refreshTracks();
}

void ArrangerView::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    if (child.hasType(ProjectState::ID_TRACK))
        refreshTracks();
}

//==============================================================================
// ScrollBar::Listener interface
//==============================================================================

void ArrangerView::scrollBarMoved(juce::ScrollBar* scrollBar, double newRangeStart)
{
    if (scrollBar == &horizontalScrollBar)
    {
        scrollOffsetX = (int)newRangeStart;
        timelineRuler->setScrollOffset(scrollOffsetX);

        for (auto* lane : trackLanes)
            lane->setScrollOffset(scrollOffsetX);
    }
    else if (scrollBar == &verticalScrollBar)
    {
        scrollOffsetY = (int)newRangeStart;
        resized();
    }
}

//==============================================================================
// Helper methods
//==============================================================================

void ArrangerView::refreshTracks()
{
    // Clear existing track lanes
    trackLanes.clear();

    // Create TrackLane for each track
    int numTracks = projectState.getNumTracks();
    for (int i = 0; i < numTracks; ++i)
    {
        auto track = projectState.getTrack(i);
        if (!track.isValid())
            continue;

        juce::String trackId = track[ProjectState::PROP_ID].toString();

        auto* lane = new TrackLane(projectState, trackId);
        lane->setPixelsPerBeat(pixelsPerBeat);
        lane->setScrollOffset(scrollOffsetX);

        trackLanes.add(lane);
        addAndMakeVisible(lane);
    }

    resized();
    repaint();
}

void ArrangerView::updateScrollBars()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(RULER_HEIGHT);
    bounds.removeFromBottom(SCROLLBAR_SIZE);
    bounds.removeFromRight(SCROLLBAR_SIZE);

    // Horizontal scrollbar (timeline length)
    // Assume max 100 bars of content
    double maxBars = 100.0;
    double beatsPerBar = projectState.getTimeSignatureNumerator();
    double totalBeats = maxBars * beatsPerBar;
    double totalWidth = totalBeats * pixelsPerBeat;

    horizontalScrollBar.setRangeLimits(0.0, totalWidth);
    horizontalScrollBar.setCurrentRange(scrollOffsetX, bounds.getWidth(), juce::dontSendNotification);
    horizontalScrollBar.setSingleStepSize(pixelsPerBeat);  // One beat
    horizontalScrollBar.setAutoHide(false);

    // Vertical scrollbar (track lanes)
    int totalHeight = trackLanes.size() * TRACK_HEIGHT;
    verticalScrollBar.setRangeLimits(0.0, totalHeight);
    verticalScrollBar.setCurrentRange(scrollOffsetY, bounds.getHeight(), juce::dontSendNotification);
    verticalScrollBar.setSingleStepSize(TRACK_HEIGHT);  // One track
    verticalScrollBar.setAutoHide(false);
}
