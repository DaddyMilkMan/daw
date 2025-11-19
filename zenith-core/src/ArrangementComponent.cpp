/**
 * @file ArrangementComponent.cpp
 * @brief Implementation of arrangement view with automation
 */

#include "../include/ArrangementComponent.h"
#include "../include/ProjectState.h"
#include "../include/Engine.h"

//==============================================================================
ArrangementComponent::ArrangementComponent(ProjectState& ps, Engine& eng)
    : projectState(ps)
    , engine(eng)
{
    // Listen to ProjectState changes
    projectState.getState().addListener(this);

    // Rebuild track UI state
    rebuildTrackUIState();

    // Enable keyboard focus
    setWantsKeyboardFocus(true);

    // Start timer for UI updates (30 Hz)
    startTimer(33);

    DBG("ArrangementComponent: Created");
}

ArrangementComponent::~ArrangementComponent()
{
    stopTimer();
    projectState.getState().removeListener(this);
    DBG("ArrangementComponent: Destroyed");
}

//==============================================================================
// Component Interface
//==============================================================================

void ArrangementComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));

    // Split into header and timeline areas
    auto headerArea = bounds.removeFromLeft(TRACK_HEADER_WIDTH);
    auto timelineArea = bounds;

    // Draw track headers
    drawTrackHeaders(g, headerArea);

    // Draw timeline
    drawTimeline(g, timelineArea);
}

void ArrangementComponent::resized()
{
    // Layout handled dynamically in paint
}

//==============================================================================
// Drawing Helpers
//==============================================================================

void ArrangementComponent::drawTrackHeaders(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    int y = 0;
    for (int i = 0; i < tracksNode.getNumChildren(); ++i)
    {
        auto track = tracksNode.getChild(i);
        auto trackName = track[ProjectState::PROP_NAME].toString();
        auto trackId = track[ProjectState::PROP_ID].toString();

        auto trackHeaderArea = juce::Rectangle<int>(area.getX(), y, area.getWidth(), TRACK_HEIGHT);

        // Header background
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRect(trackHeaderArea);

        // Border
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(trackHeaderArea, 1);

        // Track name
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);
        auto nameArea = trackHeaderArea.reduced(10, 5);
        g.drawText(trackName, nameArea.removeFromTop(20), juce::Justification::centredLeft);

        // Automation lane buttons (V, P, M)
        auto buttonArea = nameArea.removeFromTop(25).removeFromLeft(120);
        int buttonWidth = 35;
        int buttonSpacing = 5;

        auto visibleParam = getVisibleAutomationParam(i);

        // Volume button
        {
            auto vBtn = buttonArea.removeFromLeft(buttonWidth);
            bool isActive = (visibleParam == "volume");
            g.setColour(isActive ? juce::Colours::orange : juce::Colour(0xff444444));
            g.fillRect(vBtn);
            g.setColour(juce::Colours::white);
            g.drawRect(vBtn, 1);
            g.drawText("V", vBtn, juce::Justification::centred);
            buttonArea.removeFromLeft(buttonSpacing);

            // Visual indicator if automation exists
            if (projectState.hasAutomation(trackId, "volume"))
            {
                g.setColour(juce::Colours::orange);
                g.fillEllipse(vBtn.getRight() - 8, vBtn.getY() + 2, 4, 4);
            }
        }

        // Pan button
        {
            auto pBtn = buttonArea.removeFromLeft(buttonWidth);
            bool isActive = (visibleParam == "pan");
            g.setColour(isActive ? juce::Colours::cyan : juce::Colour(0xff444444));
            g.fillRect(pBtn);
            g.setColour(juce::Colours::white);
            g.drawRect(pBtn, 1);
            g.drawText("P", pBtn, juce::Justification::centred);
            buttonArea.removeFromLeft(buttonSpacing);

            if (projectState.hasAutomation(trackId, "pan"))
            {
                g.setColour(juce::Colours::cyan);
                g.fillEllipse(pBtn.getRight() - 8, pBtn.getY() + 2, 4, 4);
            }
        }

        // Mute button
        {
            auto mBtn = buttonArea.removeFromLeft(buttonWidth);
            bool isActive = (visibleParam == "mute");
            g.setColour(isActive ? juce::Colours::red : juce::Colour(0xff444444));
            g.fillRect(mBtn);
            g.setColour(juce::Colours::white);
            g.drawRect(mBtn, 1);
            g.drawText("M", mBtn, juce::Justification::centred);

            if (projectState.hasAutomation(trackId, "mute"))
            {
                g.setColour(juce::Colours::red);
                g.fillEllipse(mBtn.getRight() - 8, mBtn.getY() + 2, 4, 4);
            }
        }

        y += TRACK_HEIGHT;
    }
}

void ArrangementComponent::drawTimeline(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    int y = 0;
    for (int i = 0; i < tracksNode.getNumChildren(); ++i)
    {
        auto trackArea = juce::Rectangle<int>(area.getX(), y, area.getWidth(), TRACK_HEIGHT);
        drawTrack(g, i, trackArea);
        y += TRACK_HEIGHT;
    }
}

void ArrangementComponent::drawTrack(juce::Graphics& g, int trackIndex, juce::Rectangle<int> area)
{
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (trackIndex >= tracksNode.getNumChildren())
        return;

    auto track = tracksNode.getChild(trackIndex);
    auto trackId = track[ProjectState::PROP_ID].toString();

    // Track background (alternating colors)
    g.setColour(trackIndex % 2 == 0 ? juce::Colour(0xff252525) : juce::Colour(0xff2a2a2a));
    g.fillRect(area);

    // Track border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(area, 1);

    // Split area: clips on top, automation lane on bottom (if visible)
    auto visibleParam = getVisibleAutomationParam(trackIndex);
    auto clipsArea = area;
    auto automationArea = juce::Rectangle<int>();

    if (!visibleParam.isEmpty())
    {
        int automationHeight = (TRACK_HEIGHT * AUTOMATION_LANE_HEIGHT_RATIO) / 100;
        automationArea = clipsArea.removeFromBottom(automationHeight);

        // Draw automation lane
        drawAutomationLane(g, trackId, visibleParam, automationArea);
    }

    // TODO: Draw clips (Phase 15)
    // For now, just show a placeholder
    g.setColour(juce::Colours::grey.withAlpha(0.3f));
    g.drawText("Clips area", clipsArea, juce::Justification::centred);
}

void ArrangementComponent::drawAutomationLane(juce::Graphics& g, const juce::String& trackId,
                                              const juce::String& paramId, juce::Rectangle<int> area)
{
    // Lane background
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(area);

    // Lane border
    g.setColour(juce::Colour(0xff444444));
    g.drawRect(area, 1);

    // Get automation color
    auto color = getAutomationColour(paramId);

    // Draw reference lines
    g.setColour(color.withAlpha(0.2f));
    if (paramId == "volume" || paramId == "mute")
    {
        // Mid-line at 0.5
        int midY = area.getY() + area.getHeight() / 2;
        g.drawLine(area.getX(), midY, area.getRight(), midY, 1.0f);
    }
    else if (paramId == "pan")
    {
        // Center line at 0.0 (middle)
        int centerY = area.getY() + area.getHeight() / 2;
        g.drawLine(area.getX(), centerY, area.getRight(), centerY, 1.0f);
    }

    // Get automation points
    auto points = getAutomationPoints(trackId, paramId);
    if (points.isEmpty())
        return;

    // Draw lines between points
    juce::Path curvePath;
    bool firstPoint = true;

    for (const auto& point : points)
    {
        int x = beatsToPixels(point.timeBeats) - beatsToPixels(viewOffsetBeats) + TRACK_HEADER_WIDTH;
        int y = automationValueToPixels(point.value, area, paramId);

        if (firstPoint)
        {
            curvePath.startNewSubPath(x, y);
            firstPoint = false;
        }
        else
        {
            curvePath.lineTo(x, y);
        }
    }

    g.setColour(color);
    g.strokePath(curvePath, juce::PathStrokeType(2.0f));

    // Draw points
    for (const auto& point : points)
    {
        int x = beatsToPixels(point.timeBeats) - beatsToPixels(viewOffsetBeats) + TRACK_HEADER_WIDTH;
        int y = automationValueToPixels(point.value, area, paramId);

        float radius = 6.0f;
        bool isSelected = (selectedPoint.pointId == point.pointId);

        if (isSelected)
        {
            // Selected point: larger, highlighted
            g.setColour(juce::Colours::white);
            g.fillEllipse(x - 8, y - 8, 16, 16);
            g.setColour(color);
            g.fillEllipse(x - 6, y - 6, 12, 12);
        }
        else
        {
            // Normal point
            g.setColour(juce::Colours::white);
            g.fillEllipse(x - radius, y - radius, radius * 2, radius * 2);
            g.setColour(color);
            g.fillEllipse(x - radius + 2, y - radius + 2, (radius - 2) * 2, (radius - 2) * 2);
        }
    }
}

//==============================================================================
// Mouse Interaction
//==============================================================================

void ArrangementComponent::mouseDown(const juce::MouseEvent& event)
{
    auto pos = event.getPosition();

    // Ignore clicks in header area
    if (pos.x < TRACK_HEADER_WIDTH)
    {
        // Check if clicking on automation lane buttons
        int trackIndex = pos.y / TRACK_HEIGHT;
        auto headerArea = getTrackHeaderArea(trackIndex);

        if (headerArea.contains(pos))
        {
            auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
            if (trackIndex < tracksNode.getNumChildren())
            {
                // Calculate button positions (matching drawTrackHeaders)
                auto buttonArea = headerArea.reduced(10, 5);
                buttonArea.removeFromTop(20); // Skip track name
                buttonArea = buttonArea.removeFromTop(25).removeFromLeft(120);

                int buttonWidth = 35;
                int buttonSpacing = 5;

                auto vBtn = buttonArea.removeFromLeft(buttonWidth);
                buttonArea.removeFromLeft(buttonSpacing);
                auto pBtn = buttonArea.removeFromLeft(buttonWidth);
                buttonArea.removeFromLeft(buttonSpacing);
                auto mBtn = buttonArea.removeFromLeft(buttonWidth);

                auto currentParam = getVisibleAutomationParam(trackIndex);

                if (vBtn.contains(pos))
                {
                    setVisibleAutomationParam(trackIndex, currentParam == "volume" ? "" : "volume");
                    repaint();
                }
                else if (pBtn.contains(pos))
                {
                    setVisibleAutomationParam(trackIndex, currentParam == "pan" ? "" : "pan");
                    repaint();
                }
                else if (mBtn.contains(pos))
                {
                    setVisibleAutomationParam(trackIndex, currentParam == "mute" ? "" : "mute");
                    repaint();
                }
            }
        }
        return;
    }

    // Check if clicking in automation lane
    int trackIndex = pos.y / TRACK_HEIGHT;
    auto automationArea = getAutomationLaneArea(trackIndex);

    if (!automationArea.contains(pos))
        return;

    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (trackIndex >= tracksNode.getNumChildren())
        return;

    auto track = tracksNode.getChild(trackIndex);
    auto trackId = track[ProjectState::PROP_ID].toString();
    auto visibleParam = getVisibleAutomationParam(trackIndex);

    if (visibleParam.isEmpty())
        return;

    // Check if clicking on existing point
    juce::String hitParamId, hitPointId;
    findAutomationPointAtPosition(pos, trackIndex, hitParamId, hitPointId);

    if (!hitPointId.isEmpty())
    {
        // Start dragging existing point
        auto envelope = projectState.getAutomationEnvelope(trackId, visibleParam);
        if (envelope.isValid())
        {
            auto pointsNode = envelope.getChildWithName(ProjectState::ID_POINTS);
            for (auto point : pointsNode)
            {
                if (point[ProjectState::PROP_ID].toString() == hitPointId)
                {
                    double timeBeats = point[ProjectState::PROP_TIME_BEATS];
                    double value = point[ProjectState::PROP_VALUE];
                    startDraggingPoint(trackId, visibleParam, hitPointId, timeBeats, value);
                    return;
                }
            }
        }
    }
    else
    {
        // Add new point
        double timeBeats = pixelsToBeats(pos.x - TRACK_HEADER_WIDTH) + viewOffsetBeats;
        double value = pixelsToAutomationValue(pos.y, automationArea, visibleParam);

        timeBeats = snapToGrid(timeBeats);
        addAutomationPoint(trackId, visibleParam, timeBeats, value);
    }
}

void ArrangementComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (!selectedPoint.isDragging)
        return;

    auto pos = event.getPosition();
    int trackIndex = pos.y / TRACK_HEIGHT;
    auto automationArea = getAutomationLaneArea(trackIndex);

    double newTimeBeats = pixelsToBeats(pos.x - TRACK_HEADER_WIDTH) + viewOffsetBeats;
    double newValue = pixelsToAutomationValue(pos.y, automationArea, selectedPoint.paramId);

    newTimeBeats = snapToGrid(newTimeBeats);
    updateDraggingPoint(newTimeBeats, newValue);
    repaint();
}

void ArrangementComponent::mouseUp(const juce::MouseEvent& event)
{
    (void)event;

    if (selectedPoint.isDragging)
    {
        finishDraggingPoint();
    }
}

//==============================================================================
// Keyboard Interaction
//==============================================================================

bool ArrangementComponent::keyPressed(const juce::KeyPress& key)
{
    // Delete key
    if (key.getKeyCode() == juce::KeyPress::deleteKey ||
        key.getKeyCode() == juce::KeyPress::backspaceKey)
    {
        if (selectedPoint.isValid())
        {
            deleteSelectedPoint();
            return true;
        }
    }

    // Undo (Cmd+Z / Ctrl+Z)
    if (key.isKeyCode(juce::KeyPress::F1Key) || // Placeholder since Cmd/Ctrl handling varies
        (key.getModifiers().isCommandDown() && key.getKeyCode() == 'Z'))
    {
        projectState.undo();
        return true;
    }

    // Redo (Cmd+Shift+Z / Ctrl+Y)
    if (key.isKeyCode(juce::KeyPress::F2Key) ||
        (key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown() && key.getKeyCode() == 'Z'))
    {
        projectState.redo();
        return true;
    }

    return false;
}

//==============================================================================
// ValueTree Listener
//==============================================================================

void ArrangementComponent::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&)
{
    repaint();
}

void ArrangementComponent::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree&)
{
    // Check if tracks changed
    if (parent.getType() == ProjectState::ID_TRACKS)
    {
        rebuildTrackUIState();
    }
    repaint();
}

void ArrangementComponent::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree&, int)
{
    if (parent.getType() == ProjectState::ID_TRACKS)
    {
        rebuildTrackUIState();
    }
    repaint();
}

void ArrangementComponent::valueTreeChildOrderChanged(juce::ValueTree&, int, int)
{
    repaint();
}

void ArrangementComponent::valueTreeParentChanged(juce::ValueTree&)
{
    repaint();
}

//==============================================================================
// Timer Callback
//==============================================================================

void ArrangementComponent::timerCallback()
{
    // Check if track count changed
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    int currentTrackCount = tracksNode.isValid() ? tracksNode.getNumChildren() : 0;

    if (currentTrackCount != lastTrackCount)
    {
        lastTrackCount = currentTrackCount;
        rebuildTrackUIState();
        repaint();
    }
}

//==============================================================================
// Coordinate Mapping
//==============================================================================

double ArrangementComponent::pixelsToBeats(int pixels) const
{
    return pixels / pixelsPerBeat;
}

int ArrangementComponent::beatsToPixels(double beats) const
{
    return static_cast<int>(beats * pixelsPerBeat);
}

double ArrangementComponent::pixelsToAutomationValue(int y, juce::Rectangle<int> laneArea,
                                                     const juce::String& paramId) const
{
    double normalizedY = 1.0 - static_cast<double>(y - laneArea.getY()) / laneArea.getHeight();
    normalizedY = juce::jlimit(0.0, 1.0, normalizedY);

    // Map to parameter range
    if (paramId == "volume")
    {
        return normalizedY; // 0.0 to 1.0
    }
    else if (paramId == "pan")
    {
        return normalizedY * 2.0 - 1.0; // -1.0 to 1.0
    }
    else if (paramId == "mute")
    {
        return normalizedY > 0.5 ? 1.0 : 0.0; // 0 or 1
    }

    return normalizedY;
}

int ArrangementComponent::automationValueToPixels(double value, juce::Rectangle<int> laneArea,
                                                  const juce::String& paramId) const
{
    // Map from parameter range to 0-1
    double normalizedValue = 0.0;

    if (paramId == "volume")
    {
        normalizedValue = value; // Already 0-1
    }
    else if (paramId == "pan")
    {
        normalizedValue = (value + 1.0) / 2.0; // -1 to 1 → 0 to 1
    }
    else if (paramId == "mute")
    {
        normalizedValue = value; // 0 or 1
    }

    normalizedValue = juce::jlimit(0.0, 1.0, normalizedValue);

    // Invert Y (0 at bottom, 1 at top)
    int y = laneArea.getY() + static_cast<int>((1.0 - normalizedValue) * laneArea.getHeight());
    return y;
}

//==============================================================================
// Hit Testing
//==============================================================================

bool ArrangementComponent::isPointInAutomationLane(juce::Point<int> pos, int trackIndex) const
{
    auto laneArea = getAutomationLaneArea(trackIndex);
    return laneArea.contains(pos);
}

juce::String ArrangementComponent::findAutomationPointAtPosition(juce::Point<int> pos, int trackIndex,
                                                                 juce::String& outParamId,
                                                                 juce::String& outPointId)
{
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (trackIndex >= tracksNode.getNumChildren())
        return {};

    auto track = tracksNode.getChild(trackIndex);
    auto trackId = track[ProjectState::PROP_ID].toString();
    auto visibleParam = getVisibleAutomationParam(trackIndex);

    if (visibleParam.isEmpty())
        return {};

    auto automationArea = getAutomationLaneArea(trackIndex);
    auto points = getAutomationPoints(trackId, visibleParam);

    const int hitRadius = 8;

    for (const auto& point : points)
    {
        int x = beatsToPixels(point.timeBeats) - beatsToPixels(viewOffsetBeats) + TRACK_HEADER_WIDTH;
        int y = automationValueToPixels(point.value, automationArea, visibleParam);

        int dx = pos.x - x;
        int dy = pos.y - y;
        int distSq = dx * dx + dy * dy;

        if (distSq <= hitRadius * hitRadius)
        {
            outParamId = visibleParam;
            outPointId = point.pointId;
            return trackId;
        }
    }

    return {};
}

juce::Rectangle<int> ArrangementComponent::getTrackArea(int trackIndex) const
{
    return juce::Rectangle<int>(0, trackIndex * TRACK_HEIGHT, getWidth(), TRACK_HEIGHT);
}

juce::Rectangle<int> ArrangementComponent::getAutomationLaneArea(int trackIndex) const
{
    auto visibleParam = getVisibleAutomationParam(trackIndex);
    if (visibleParam.isEmpty())
        return {};

    int automationHeight = (TRACK_HEIGHT * AUTOMATION_LANE_HEIGHT_RATIO) / 100;
    int y = trackIndex * TRACK_HEIGHT + TRACK_HEIGHT - automationHeight;

    return juce::Rectangle<int>(TRACK_HEADER_WIDTH, y, getWidth() - TRACK_HEADER_WIDTH, automationHeight);
}

juce::Rectangle<int> ArrangementComponent::getTrackHeaderArea(int trackIndex) const
{
    return juce::Rectangle<int>(0, trackIndex * TRACK_HEIGHT, TRACK_HEADER_WIDTH, TRACK_HEIGHT);
}

//==============================================================================
// Automation Editing
//==============================================================================

void ArrangementComponent::addAutomationPoint(const juce::String& trackId, const juce::String& paramId,
                                              double timeBeats, double value)
{
    auto pointId = projectState.addAutomationPoint(trackId, paramId, timeBeats, value, "Add automation point");

    // Select the new point
    selectedPoint.trackId = trackId;
    selectedPoint.paramId = paramId;
    selectedPoint.pointId = pointId;
    selectedPoint.originalTimeBeats = timeBeats;
    selectedPoint.originalValue = value;
    selectedPoint.isDragging = false;

    repaint();
}

void ArrangementComponent::startDraggingPoint(const juce::String& trackId, const juce::String& paramId,
                                              const juce::String& pointId, double timeBeats, double value)
{
    selectedPoint.trackId = trackId;
    selectedPoint.paramId = paramId;
    selectedPoint.pointId = pointId;
    selectedPoint.originalTimeBeats = timeBeats;
    selectedPoint.originalValue = value;
    selectedPoint.isDragging = true;
}

void ArrangementComponent::updateDraggingPoint(double newTimeBeats, double newValue)
{
    // During drag, we just update the display
    // The actual ProjectState update happens on mouseUp
    // This prevents spamming the undo manager

    // For now, we'll update in real-time (simpler MVP)
    // A more sophisticated version would use a transient preview
    projectState.moveAutomationPoint(selectedPoint.trackId, selectedPoint.paramId,
                                     selectedPoint.pointId, newTimeBeats, newValue,
                                     "Move automation point");
}

void ArrangementComponent::finishDraggingPoint()
{
    selectedPoint.isDragging = false;
    repaint();
}

void ArrangementComponent::deleteSelectedPoint()
{
    if (!selectedPoint.isValid())
        return;

    projectState.deleteAutomationPoint(selectedPoint.trackId, selectedPoint.paramId,
                                       selectedPoint.pointId, "Delete automation point");

    selectedPoint.clear();
    repaint();
}

//==============================================================================
// Track UI State Management
//==============================================================================

void ArrangementComponent::rebuildTrackUIState()
{
    trackUIStates.clear();

    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    for (int i = 0; i < tracksNode.getNumChildren(); ++i)
    {
        auto track = tracksNode.getChild(i);
        TrackUIState state;
        state.trackId = track[ProjectState::PROP_ID].toString();
        state.trackIndex = i;
        state.visibleAutomationParam = ""; // Default: no automation visible
        trackUIStates.push_back(state);
    }

    DBG("ArrangementComponent: Rebuilt UI state for " + juce::String(trackUIStates.size()) + " tracks");
}

void ArrangementComponent::setVisibleAutomationParam(int trackIndex, const juce::String& paramId)
{
    if (trackIndex >= 0 && trackIndex < static_cast<int>(trackUIStates.size()))
    {
        trackUIStates[trackIndex].visibleAutomationParam = paramId;
    }
}

juce::String ArrangementComponent::getVisibleAutomationParam(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < static_cast<int>(trackUIStates.size()))
    {
        return trackUIStates[trackIndex].visibleAutomationParam;
    }
    return {};
}

//==============================================================================
// Helpers
//==============================================================================

juce::Array<AutomationPointView> ArrangementComponent::getAutomationPoints(const juce::String& trackId,
                                                                            const juce::String& paramId) const
{
    juce::Array<AutomationPointView> result;

    auto envelope = projectState.getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
        return result;

    auto pointsNode = envelope.getChildWithName(ProjectState::ID_POINTS);
    if (!pointsNode.isValid())
        return result;

    for (auto point : pointsNode)
    {
        AutomationPointView view;
        view.pointId = point[ProjectState::PROP_ID].toString();
        view.timeBeats = point[ProjectState::PROP_TIME_BEATS];
        view.value = point[ProjectState::PROP_VALUE];
        result.add(view);
    }

    return result;
}

juce::Colour ArrangementComponent::getAutomationColour(const juce::String& paramId) const
{
    if (paramId == "volume")
        return juce::Colours::orange;
    else if (paramId == "pan")
        return juce::Colours::cyan;
    else if (paramId == "mute")
        return juce::Colours::red;

    return juce::Colours::white;
}

double ArrangementComponent::snapToGrid(double beats) const
{
    double gridSize = 0.25; // 1/16 beat
    return std::round(beats / gridSize) * gridSize;
}

