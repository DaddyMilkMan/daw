/**
 * @file ArrangerView.cpp
 * @brief ArrangerView implementation (integration stub)
 */

#include "../include/ArrangerView.h"

//==============================================================================
// ArrangerView Implementation
//==============================================================================

ArrangerView::ArrangerView(ProjectState& ps)
    : projectState(ps)
{
    DBG("ArrangerView: Constructor");

    // Listen to ProjectState changes
    projectState.getState().addListener(this);

    // Build initial track display
    rebuildTracks();
}

ArrangerView::~ArrangerView()
{
    projectState.getState().removeListener(this);
    DBG("ArrangerView: Destructor");
}

//==============================================================================
void ArrangerView::setTrackAutomationVisible(const juce::String& trackId, bool show)
{
    trackAutomationVisible[trackId] = show;

    if (show)
    {
        // Create automation lane component if it doesn't exist
        if (automationLanes.find(trackId) == automationLanes.end())
        {
            // For now, default to volume automation
            auto lane = std::make_unique<AutomationLaneComponent>(projectState, trackId, "volume");
            addAndMakeVisible(lane.get());
            automationLanes[trackId] = std::move(lane);
            DBG("ArrangerView: Created automation lane for track " + trackId);
        }
    }
    else
    {
        // Remove automation lane component
        auto it = automationLanes.find(trackId);
        if (it != automationLanes.end())
        {
            removeChildComponent(it->second.get());
            automationLanes.erase(it);
            DBG("ArrangerView: Removed automation lane for track " + trackId);
        }
    }

    resized();
}

bool ArrangerView::isTrackAutomationVisible(const juce::String& trackId) const
{
    auto it = trackAutomationVisible.find(trackId);
    return it != trackAutomationVisible.end() && it->second;
}

//==============================================================================
void ArrangerView::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Draw placeholder content
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(16.0f));
    // [STUB AUDIT] Removed "Integration Stub" text as immediate mode rendering is active
    // g.drawText("ArrangerView (Integration Stub)", ...

    // Draw grid lines (beat markers)
    g.setColour(juce::Colour(0xff3a3a3a));
    for (int beat = 0; beat < 64; ++beat)
    {
        float x = beatsToPixels(static_cast<double>(beat));
        g.drawVerticalLine(static_cast<int>(x), 0.0f, static_cast<float>(getHeight()));
    }

    // Draw tracks from ProjectState
    auto& state = projectState.getState();
    auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

    if (tracksNode.isValid())
    {
        int trackY = 40;
        for (const auto& track : tracksNode)
        {
            juce::String trackId = track[ProjectState::PROP_ID].toString();
            juce::String trackName = track[ProjectState::PROP_NAME].toString();

            // Track background
            juce::Rectangle<int> trackRect(0, trackY, getWidth(), trackHeight);
            g.setColour(juce::Colour(0xff1e1e1e));
            g.fillRect(trackRect);

            // Track border
            g.setColour(juce::Colour(0xff3a3a3a));
            g.drawRect(trackRect);

            // Track name
            g.setColour(juce::Colours::white);
            g.setFont(juce::FontOptions(14.0f));
            g.drawText(trackName, trackRect.reduced(10, 5), juce::Justification::topLeft);

            // Draw clips
            auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);
            if (clipsNode.isValid())
            {
                for (const auto& clip : clipsNode)
                {
                    double startBeats = clip[ProjectState::PROP_START];
                    double lengthBeats = clip[ProjectState::PROP_LENGTH];
                    juce::String clipType = clip[ProjectState::PROP_TYPE].toString();

                    float x = beatsToPixels(startBeats);
                    float width = beatsToPixels(lengthBeats);

                    juce::Rectangle<float> clipRect(x, static_cast<float>(trackY + 25),
                                                     width, static_cast<float>(trackHeight - 30));

                    // Clip background (blue for MIDI, green for audio)
                    g.setColour(clipType == "midi" ? juce::Colours::blue : juce::Colours::green);
                    g.fillRect(clipRect);

                    // Clip border
                    g.setColour(juce::Colours::white);
                    g.drawRect(clipRect, 1.0f);
                }
            }

            trackY += trackHeight;

            // Draw automation lane if visible
            if (isTrackAutomationVisible(trackId))
            {
                trackY += automationLaneHeight;
            }
        }
    }
}

void ArrangerView::resized()
{
    // Layout automation lanes
    auto& state = projectState.getState();
    auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

    if (tracksNode.isValid())
    {
        int trackY = 40;
        for (const auto& track : tracksNode)
        {
            juce::String trackId = track[ProjectState::PROP_ID].toString();

            trackY += trackHeight;

            // Position automation lane if visible
            if (isTrackAutomationVisible(trackId))
            {
                auto it = automationLanes.find(trackId);
                if (it != automationLanes.end())
                {
                    it->second->setBounds(0, trackY, getWidth(), automationLaneHeight);
                }
                trackY += automationLaneHeight;
            }
        }
    }
}

void ArrangerView::mouseDown(const juce::MouseEvent& event)
{
    DBG("ArrangerView: mouseDown at " + event.position.toString());
}

void ArrangerView::mouseDoubleClick(const juce::MouseEvent& event)
{
    // Find clip at position
    auto [trackId, clipId] = findClipAtPosition(event.position.toInt());

    if (trackId.isNotEmpty() && clipId.isNotEmpty())
    {
        // Check if it's a MIDI clip
        auto& state = projectState.getState();
        auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

        for (const auto& track : tracksNode)
        {
            if (track[ProjectState::PROP_ID].toString() == trackId)
            {
                auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);
                for (const auto& clip : clipsNode)
                {
                    if (clip[ProjectState::PROP_ID].toString() == clipId)
                    {
                        juce::String clipType = clip[ProjectState::PROP_TYPE].toString();

                        if (clipType == "midi" && openPianoRollCallback)
                        {
                            DBG("ArrangerView: Opening piano roll for " + trackId + "/" + clipId);
                            openPianoRollCallback(trackId, clipId);
                        }
                        return;
                    }
                }
            }
        }
    }
}

//==============================================================================
void ArrangerView::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    juce::ignoreUnused(tree, property);
    repaint();
}

void ArrangerView::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    juce::ignoreUnused(parent, child);
    rebuildTracks();
    repaint();
}

void ArrangerView::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    juce::ignoreUnused(parent, child, index);
    rebuildTracks();
    repaint();
}

//==============================================================================
void ArrangerView::rebuildTracks()
{
    // [STUB AUDIT] Immediate Mode Rendering Active
    // We are not creating child components for tracks in this phase.
    // Instead, we render everything in paint() for performance and simplicity.
    // This method is kept for future expansion to Component-based tracks.
    repaint();
}

std::pair<juce::String, juce::String> ArrangerView::findClipAtPosition(juce::Point<int> position)
{
    // Integration stub: Find clip at pixel position
    auto& state = projectState.getState();
    auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

    if (tracksNode.isValid())
    {
        int trackY = 40;
        for (const auto& track : tracksNode)
        {
            if (position.y >= trackY && position.y < trackY + trackHeight)
            {
                juce::String trackId = track[ProjectState::PROP_ID].toString();

                // Check clips
                auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);
                if (clipsNode.isValid())
                {
                    double clickBeats = pixelsToBeats(static_cast<float>(position.x));

                    for (const auto& clip : clipsNode)
                    {
                        double startBeats = clip[ProjectState::PROP_START];
                        double lengthBeats = clip[ProjectState::PROP_LENGTH];

                        if (clickBeats >= startBeats && clickBeats < startBeats + lengthBeats)
                        {
                            return {trackId, clip[ProjectState::PROP_ID].toString()};
                        }
                    }
                }
            }

            trackY += trackHeight;
            if (isTrackAutomationVisible(track[ProjectState::PROP_ID].toString()))
            {
                trackY += automationLaneHeight;
            }
        }
    }

    return {{}, {}};
}

//==============================================================================
float ArrangerView::beatsToPixels(double beats) const
{
    return static_cast<float>(beats * pixelsPerBeat);
}

double ArrangerView::pixelsToBeats(float pixels) const
{
    return static_cast<double>(pixels) / pixelsPerBeat;
}

//==============================================================================
// AutomationLaneComponent Implementation
//==============================================================================

AutomationLaneComponent::AutomationLaneComponent(ProjectState& ps,
                                                 const juce::String& tid,
                                                 const juce::String& pid)
    : projectState(ps), trackId(tid), paramId(pid)
{
    DBG("AutomationLaneComponent: Constructor for " + trackId + "/" + paramId);
}

AutomationLaneComponent::~AutomationLaneComponent()
{
    DBG("AutomationLaneComponent: Destructor");
}

//==============================================================================
void AutomationLaneComponent::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff252525));

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(getLocalBounds());

    // Label
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(paramId + " automation", getLocalBounds().reduced(5), juce::Justification::topLeft);

    // Get automation envelope from ProjectState
    auto envelope = projectState.getAutomationEnvelope(trackId, paramId);

    if (envelope.isValid())
    {
        // Draw automation points
        g.setColour(juce::Colours::yellow);

        for (const auto& point : envelope)
        {
            double timeBeats = point[ProjectState::PROP_TIME_BEATS];
            double value = point[ProjectState::PROP_VALUE];

            // Convert to pixel coordinates (simplified - would use ArrangerView's beatsToPixels)
            float x = static_cast<float>(timeBeats * 40.0);  // 40 pixels per beat
            float y = valueToY(value);

            g.fillEllipse(x - 3.0f, y - 3.0f, 6.0f, 6.0f);
        }

        // Draw curve between points (simplified - would use proper interpolation)
        g.setColour(juce::Colours::yellow.withAlpha(0.5f));
        juce::Path path;
        bool firstPoint = true;

        for (const auto& point : envelope)
        {
            double timeBeats = point[ProjectState::PROP_TIME_BEATS];
            double value = point[ProjectState::PROP_VALUE];

            float x = static_cast<float>(timeBeats * 40.0);
            float y = valueToY(value);

            if (firstPoint)
            {
                path.startNewSubPath(x, y);
                firstPoint = false;
            }
            else
            {
                path.lineTo(x, y);
            }
        }

        g.strokePath(path, juce::PathStrokeType(2.0f));
    }
    else
    {
        // No automation yet
        g.setColour(juce::Colours::grey);
        g.drawText("Click to add automation points",
                   getLocalBounds(),
                   juce::Justification::centred);
    }
}

void AutomationLaneComponent::resized()
{
}

void AutomationLaneComponent::mouseDown(const juce::MouseEvent& event)
{
    // Integration stub: Add automation point or select existing point
    double timeBeats = static_cast<double>(event.x) / 40.0;  // 40 pixels per beat
    double value = yToValue(static_cast<float>(event.y));

    DBG("AutomationLaneComponent: Add point at " + juce::String(timeBeats) +
        " beats, value " + juce::String(value));

    // Add point to ProjectState (TrackAutomationSynchronizer will pick it up)
    projectState.addAutomationPoint(trackId, paramId, timeBeats, value, "Add automation point");

    repaint();
}

void AutomationLaneComponent::mouseDrag(const juce::MouseEvent& event)
{
    // [STUB AUDIT] Dragging automation points requires hit-testing existing points
    // which is not yet implemented in the immediate mode renderer.
    // For now, we log this action.
    juce::ignoreUnused(event);
    // DBG("AutomationLaneComponent: Drag not yet implemented");
}

//==============================================================================
float AutomationLaneComponent::valueToY(double value) const
{
    // Convert value to Y coordinate (inverted - 0 at top, 1 at bottom)
    // For pan: -1 to 1 → 0 to height
    // For volume/mute: 0 to 1 → 0 to height

    float normalizedValue;
    if (paramId == "pan")
    {
        normalizedValue = static_cast<float>((value + 1.0) / 2.0);  // -1..1 → 0..1
    }
    else
    {
        normalizedValue = static_cast<float>(value);  // Already 0..1
    }

    return static_cast<float>(getHeight()) * (1.0f - normalizedValue);
}

double AutomationLaneComponent::yToValue(float y) const
{
    // Convert Y coordinate to value
    float normalizedValue = 1.0f - (y / static_cast<float>(getHeight()));
    normalizedValue = juce::jlimit(0.0f, 1.0f, normalizedValue);

    if (paramId == "pan")
    {
        return static_cast<double>(normalizedValue * 2.0 - 1.0);  // 0..1 → -1..1
    }
    else
    {
        return static_cast<double>(normalizedValue);
    }
}

