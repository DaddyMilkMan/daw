/**
 * @file ArrangerComponent.cpp
 * @brief Implementation of ArrangerComponent for Phase 14
 *
 * Phase 14: Arranger Automation Lanes UI
 */

#include "ArrangerComponent.h"
#include <algorithm>
#include <cmath>

//==============================================================================
ArrangerComponent::ArrangerComponent(ProjectState& ps)
    : projectState(ps)
{
    // Listen to ValueTree changes
    projectState.getState().addListener(this);

    // Add as key listener to handle Delete key
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    // Set size
    setSize(800, 600);
}

ArrangerComponent::~ArrangerComponent()
{
    projectState.getState().removeListener(this);
}

//==============================================================================
// Component Interface
//==============================================================================

void ArrangerComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2b2b2b)); // Dark background

    // Get tracks
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
    {
        g.setColour(juce::Colours::white);
        g.drawText("No tracks - use CommandAPI to add tracks", getLocalBounds(), juce::Justification::centred);
        return;
    }

    int numTracks = tracksNode.getNumChildren();

    // Draw each track
    for (int i = 0; i < numTracks; ++i)
    {
        auto trackNode = tracksNode.getChild(i);
        juce::String trackId = trackNode[ProjectState::PROP_ID].toString();

        // Get track bounds
        auto trackBounds = getTrackLaneBounds(i);

        // Draw track background
        g.setColour(juce::Colour(0xff3a3a3a));
        g.fillRect(trackBounds);

        // Draw track border
        g.setColour(juce::Colour(0xff505050));
        g.drawRect(trackBounds);

        // Split into header and timeline area
        auto headerArea = trackBounds.removeFromLeft(TRACK_HEADER_WIDTH);
        auto timelineArea = trackBounds;

        // Paint track header
        paintTrackHeader(g, i, headerArea);

        // Paint automation lanes
        auto& uiState = getOrCreateUIState(trackId);
        int laneOffset = 0;

        if (uiState.showVolumeLane)
        {
            auto laneArea = getAutomationLaneBounds(i, "volume");
            paintAutomationLane(g, i, "volume", laneArea);
            laneOffset++;
        }

        if (uiState.showPanLane)
        {
            auto laneArea = getAutomationLaneBounds(i, "pan");
            paintAutomationLane(g, i, "pan", laneArea);
            laneOffset++;
        }

        if (uiState.showMuteLane)
        {
            auto laneArea = getAutomationLaneBounds(i, "mute");
            paintAutomationLane(g, i, "mute", laneArea);
            laneOffset++;
        }
    }
}

void ArrangerComponent::resized()
{
    // Layout is done in paint() dynamically
}

//==============================================================================
// Track Header Painting
//==============================================================================

void ArrangerComponent::paintTrackHeader(juce::Graphics& g, int trackIndex, const juce::Rectangle<int>& area)
{
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid() || trackIndex >= tracksNode.getNumChildren())
        return;

    auto trackNode = tracksNode.getChild(trackIndex);
    juce::String trackName = trackNode[ProjectState::PROP_NAME].toString();
    juce::String trackId = trackNode[ProjectState::PROP_ID].toString();

    auto& uiState = getOrCreateUIState(trackId);

    // Draw background
    g.setColour(juce::Colour(0xff454545));
    g.fillRect(area);

    // Draw track name
    auto nameArea = area.reduced(4).removeFromTop(24);
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText(trackName, nameArea, juce::Justification::centredLeft);

    // Draw V/P/M toggle buttons
    auto buttonArea = area.reduced(4);
    buttonArea.removeFromTop(28); // Skip name area

    int buttonY = buttonArea.getY();
    int buttonX = buttonArea.getX();

    // Volume button
    {
        juce::Rectangle<int> btn(buttonX, buttonY, TOGGLE_BUTTON_SIZE, TOGGLE_BUTTON_SIZE);
        g.setColour(uiState.showVolumeLane ? juce::Colour(0xff4a90e2) : juce::Colour(0xff666666));
        g.fillRect(btn);
        g.setColour(juce::Colours::white);
        g.drawRect(btn);
        g.setFont(12.0f);
        g.drawText("V", btn, juce::Justification::centred);
        buttonX += TOGGLE_BUTTON_SIZE + 4;
    }

    // Pan button
    {
        juce::Rectangle<int> btn(buttonX, buttonY, TOGGLE_BUTTON_SIZE, TOGGLE_BUTTON_SIZE);
        g.setColour(uiState.showPanLane ? juce::Colour(0xff4a90e2) : juce::Colour(0xff666666));
        g.fillRect(btn);
        g.setColour(juce::Colours::white);
        g.drawRect(btn);
        g.setFont(12.0f);
        g.drawText("P", btn, juce::Justification::centred);
        buttonX += TOGGLE_BUTTON_SIZE + 4;
    }

    // Mute button
    {
        juce::Rectangle<int> btn(buttonX, buttonY, TOGGLE_BUTTON_SIZE, TOGGLE_BUTTON_SIZE);
        g.setColour(uiState.showMuteLane ? juce::Colour(0xff4a90e2) : juce::Colour(0xff666666));
        g.fillRect(btn);
        g.setColour(juce::Colours::white);
        g.drawRect(btn);
        g.setFont(12.0f);
        g.drawText("M", btn, juce::Justification::centred);
    }
}

//==============================================================================
// Automation Lane Painting
//==============================================================================

void ArrangerComponent::paintAutomationLane(juce::Graphics& g, int trackIndex, const juce::String& param,
                                             const juce::Rectangle<float>& laneArea)
{
    // Get track ID
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid() || trackIndex >= tracksNode.getNumChildren())
        return;

    auto trackNode = tracksNode.getChild(trackIndex);
    juce::String trackId = trackNode[ProjectState::PROP_ID].toString();

    // Draw lane background
    g.setColour(juce::Colour(0xff2f2f2f));
    g.fillRect(laneArea);

    // Draw lane border
    g.setColour(juce::Colour(0xff555555));
    g.drawRect(laneArea);

    // Draw center line (for pan) or baseline (for volume)
    if (param == "pan")
    {
        float centerY = laneArea.getCentreY();
        g.setColour(juce::Colour(0xff666666));
        g.drawHorizontalLine((int)centerY, laneArea.getX(), laneArea.getRight());
    }
    else
    {
        // Draw baseline at bottom for volume/mute
        g.setColour(juce::Colour(0xff666666));
        g.drawHorizontalLine((int)laneArea.getBottom() - 1, laneArea.getX(), laneArea.getRight());
    }

    // Draw parameter label
    g.setColour(juce::Colours::lightgrey);
    g.setFont(11.0f);
    g.drawText(param, laneArea.reduced(4.0f), juce::Justification::topLeft);

    // Get or rebuild automation curve
    auto& curve = getOrCreateCurveView(trackId, param);
    if (curve.needsRebuild)
    {
        rebuildAutomationCurve(trackId, param);
        curve.needsRebuild = false;
    }

    // Paint curve and points
    if (!curve.points.empty())
    {
        paintAutomationCurve(g, curve, laneArea, param);
        paintAutomationPoints(g, curve, laneArea, param);
    }
}

void ArrangerComponent::paintAutomationCurve(juce::Graphics& g, const AutomationCurveView& curve,
                                              const juce::Rectangle<float>& laneArea, const juce::String& param)
{
    if (curve.points.size() < 2)
        return;

    // Draw curve as line segments
    juce::Path path;

    for (size_t i = 0; i < curve.points.size(); ++i)
    {
        const auto& point = curve.points[i];
        float x = beatToX(point.timeBeats);
        float y = valueToY(point.value, param, laneArea);

        if (i == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }

    g.setColour(juce::Colour(0xff4a90e2));
    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void ArrangerComponent::paintAutomationPoints(juce::Graphics& g, const AutomationCurveView& curve,
                                               const juce::Rectangle<float>& laneArea, const juce::String& param)
{
    for (const auto& point : curve.points)
    {
        float x = beatToX(point.timeBeats);
        float y = valueToY(point.value, param, laneArea);

        // Check if this point is selected
        bool isSelected = (selectedPoint.isValid &&
                          selectedPoint.trackId == curve.trackId &&
                          selectedPoint.param == curve.param &&
                          selectedPoint.pointId == point.id);

        // Check if this point is hovered
        bool isHovered = (hoverPointId == point.id &&
                         hoverTrackId == curve.trackId &&
                         hoverParam == curve.param);

        // Draw point
        if (isSelected)
        {
            g.setColour(juce::Colour(0xffffaa00)); // Orange for selected
            g.fillEllipse(x - POINT_RADIUS - 1, y - POINT_RADIUS - 1,
                         (POINT_RADIUS + 1) * 2, (POINT_RADIUS + 1) * 2);
        }
        else if (isHovered)
        {
            g.setColour(juce::Colour(0xffffffff)); // White for hover
            g.fillEllipse(x - POINT_RADIUS, y - POINT_RADIUS, POINT_RADIUS * 2, POINT_RADIUS * 2);
        }
        else
        {
            g.setColour(juce::Colour(0xff4a90e2)); // Blue for normal
            g.fillEllipse(x - POINT_RADIUS, y - POINT_RADIUS, POINT_RADIUS * 2, POINT_RADIUS * 2);
        }

        // Draw point border
        g.setColour(juce::Colours::white);
        g.drawEllipse(x - POINT_RADIUS, y - POINT_RADIUS, POINT_RADIUS * 2, POINT_RADIUS * 2, 1.0f);
    }
}

//==============================================================================
// Coordinate Conversion
//==============================================================================

float ArrangerComponent::beatToX(double beats) const
{
    return TRACK_HEADER_WIDTH + static_cast<float>(beats * PIXELS_PER_BEAT);
}

double ArrangerComponent::xToBeat(float x) const
{
    return (x - TRACK_HEADER_WIDTH) / PIXELS_PER_BEAT;
}

float ArrangerComponent::valueToY(double value, const juce::String& param, const juce::Rectangle<float>& laneArea) const
{
    if (param == "volume" || param == "mute")
    {
        // 0 at bottom, 1 at top
        float normalized = juce::jlimit(0.0, 1.0, value);
        return laneArea.getBottom() - (normalized * laneArea.getHeight());
    }
    else if (param == "pan")
    {
        // -1 at bottom, +1 at top
        float normalized = (value + 1.0f) / 2.0f; // Map [-1, 1] to [0, 1]
        normalized = juce::jlimit(0.0f, 1.0f, normalized);
        return laneArea.getBottom() - (normalized * laneArea.getHeight());
    }

    return laneArea.getCentreY();
}

double ArrangerComponent::yToValue(float y, const juce::String& param, const juce::Rectangle<float>& laneArea) const
{
    // Invert Y (bottom = 0, top = 1)
    float normalized = (laneArea.getBottom() - y) / laneArea.getHeight();
    normalized = juce::jlimit(0.0f, 1.0f, normalized);

    if (param == "volume" || param == "mute")
    {
        return normalized;
    }
    else if (param == "pan")
    {
        // Map [0, 1] back to [-1, 1]
        return (normalized * 2.0) - 1.0;
    }

    return 0.0;
}

double ArrangerComponent::snapToGrid(double beats) const
{
    return std::round(beats / GRID_SNAP) * GRID_SNAP;
}

//==============================================================================
// Track Lane Management
//==============================================================================

juce::Rectangle<int> ArrangerComponent::getTrackLaneBounds(int trackIndex) const
{
    int y = 0;

    // Calculate Y position by summing heights of all previous tracks
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return {};

    for (int i = 0; i < trackIndex; ++i)
    {
        y += getTrackHeight(i);
    }

    int height = getTrackHeight(trackIndex);

    return juce::Rectangle<int>(0, y, getWidth(), height);
}

juce::Rectangle<float> ArrangerComponent::getAutomationLaneBounds(int trackIndex, const juce::String& param) const
{
    auto trackBounds = getTrackLaneBounds(trackIndex);

    // Get track ID
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid() || trackIndex >= tracksNode.getNumChildren())
        return {};

    auto trackNode = tracksNode.getChild(trackIndex);
    juce::String trackId = trackNode[ProjectState::PROP_ID].toString();

    // Find which lane this is (volume first, then pan, then mute)
    auto* uiState = const_cast<ArrangerComponent*>(this)->findUIState(trackId);
    if (!uiState)
        return {};

    int laneIndex = 0;
    if (param == "volume" && uiState->showVolumeLane)
    {
        laneIndex = 0;
    }
    else if (param == "pan" && uiState->showPanLane)
    {
        laneIndex = uiState->showVolumeLane ? 1 : 0;
    }
    else if (param == "mute" && uiState->showMuteLane)
    {
        laneIndex = 0;
        if (uiState->showVolumeLane) laneIndex++;
        if (uiState->showPanLane) laneIndex++;
    }
    else
    {
        return {};
    }

    // Lane area starts after base track content
    float laneY = trackBounds.getY() + TRACK_BASE_HEIGHT + (laneIndex * AUTOMATION_LANE_HEIGHT);
    float laneX = TRACK_HEADER_WIDTH;

    return juce::Rectangle<float>(laneX, laneY,
                                   static_cast<float>(getWidth() - TRACK_HEADER_WIDTH),
                                   static_cast<float>(AUTOMATION_LANE_HEIGHT));
}

int ArrangerComponent::getTrackIndexAtY(int y) const
{
    int currentY = 0;
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return -1;

    int numTracks = tracksNode.getNumChildren();

    for (int i = 0; i < numTracks; ++i)
    {
        int height = getTrackHeight(i);
        if (y >= currentY && y < currentY + height)
            return i;
        currentY += height;
    }

    return -1;
}

int ArrangerComponent::getTrackHeight(int trackIndex) const
{
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid() || trackIndex >= tracksNode.getNumChildren())
        return TRACK_BASE_HEIGHT;

    auto trackNode = tracksNode.getChild(trackIndex);
    juce::String trackId = trackNode[ProjectState::PROP_ID].toString();

    // Find UI state (const-correct version)
    int numLanes = 0;
    for (const auto& state : trackUIStates)
    {
        if (state.trackId == trackId)
        {
            if (state.showVolumeLane) numLanes++;
            if (state.showPanLane) numLanes++;
            if (state.showMuteLane) numLanes++;
            break;
        }
    }

    return TRACK_BASE_HEIGHT + (numLanes * AUTOMATION_LANE_HEIGHT);
}

void ArrangerComponent::toggleAutomationLane(const juce::String& trackId, const juce::String& param)
{
    auto& uiState = getOrCreateUIState(trackId);

    if (param == "volume")
        uiState.showVolumeLane = !uiState.showVolumeLane;
    else if (param == "pan")
        uiState.showPanLane = !uiState.showPanLane;
    else if (param == "mute")
        uiState.showMuteLane = !uiState.showMuteLane;

    repaint();
}

TrackAutomationUIState& ArrangerComponent::getOrCreateUIState(const juce::String& trackId)
{
    // Find existing
    for (auto& state : trackUIStates)
    {
        if (state.trackId == trackId)
            return state;
    }

    // Create new
    TrackAutomationUIState newState;
    newState.trackId = trackId;
    trackUIStates.push_back(newState);
    return trackUIStates.back();
}

TrackAutomationUIState* ArrangerComponent::findUIState(const juce::String& trackId)
{
    for (auto& state : trackUIStates)
    {
        if (state.trackId == trackId)
            return &state;
    }
    return nullptr;
}

//==============================================================================
// Automation Curve Cache
//==============================================================================

void ArrangerComponent::rebuildAutomationCurve(const juce::String& trackId, const juce::String& param)
{
    auto& curve = getOrCreateCurveView(trackId, param);
    curve.points.clear();

    // Get envelope from ProjectState
    auto envelope = projectState.getAutomationEnvelope(trackId, param);
    if (!envelope.isValid())
        return;

    // Build point list
    for (int i = 0; i < envelope.getNumChildren(); ++i)
    {
        auto pointNode = envelope.getChild(i);
        if (pointNode.hasType(ProjectState::ID_POINT))
        {
            juce::String id = pointNode[ProjectState::PROP_ID].toString();
            double timeBeats = pointNode[ProjectState::PROP_TIME_BEATS];
            double value = pointNode[ProjectState::PROP_VALUE];

            curve.points.emplace_back(id, timeBeats, value);
        }
    }

    // Sort by time
    std::sort(curve.points.begin(), curve.points.end(),
              [](const AutomationPointView& a, const AutomationPointView& b) {
                  return a.timeBeats < b.timeBeats;
              });

    curve.needsRebuild = false;
}

AutomationCurveView& ArrangerComponent::getOrCreateCurveView(const juce::String& trackId, const juce::String& param)
{
    // Find existing
    for (auto& curve : curveCache)
    {
        if (curve.trackId == trackId && curve.param == param)
            return curve;
    }

    // Create new
    curveCache.emplace_back(trackId, param);
    curveCache.back().needsRebuild = true;
    return curveCache.back();
}

AutomationCurveView* ArrangerComponent::findCurveView(const juce::String& trackId, const juce::String& param)
{
    for (auto& curve : curveCache)
    {
        if (curve.trackId == trackId && curve.param == param)
            return &curve;
    }
    return nullptr;
}

void ArrangerComponent::markAllCurvesDirty()
{
    for (auto& curve : curveCache)
    {
        curve.needsRebuild = true;
    }
}

//==============================================================================
// Mouse Interaction
//==============================================================================

void ArrangerComponent::mouseDown(const juce::MouseEvent& event)
{
    auto pos = event.position;

    // Check if click is in track header (V/P/M buttons)
    if (pos.x < TRACK_HEADER_WIDTH)
    {
        int trackIndex = getTrackIndexAtY((int)pos.y);
        if (trackIndex >= 0)
        {
            auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
            if (tracksNode.isValid() && trackIndex < tracksNode.getNumChildren())
            {
                auto trackNode = tracksNode.getChild(trackIndex);
                juce::String trackId = trackNode[ProjectState::PROP_ID].toString();

                auto trackBounds = getTrackLaneBounds(trackIndex);
                auto headerArea = trackBounds.removeFromLeft(TRACK_HEADER_WIDTH).reduced(4);
                headerArea.removeFromTop(28); // Skip name

                int buttonY = headerArea.getY();
                int buttonX = headerArea.getX();

                // Check volume button
                juce::Rectangle<int> vBtn(buttonX, buttonY, TOGGLE_BUTTON_SIZE, TOGGLE_BUTTON_SIZE);
                if (vBtn.contains(pos.toInt()))
                {
                    toggleAutomationLane(trackId, "volume");
                    return;
                }
                buttonX += TOGGLE_BUTTON_SIZE + 4;

                // Check pan button
                juce::Rectangle<int> pBtn(buttonX, buttonY, TOGGLE_BUTTON_SIZE, TOGGLE_BUTTON_SIZE);
                if (pBtn.contains(pos.toInt()))
                {
                    toggleAutomationLane(trackId, "pan");
                    return;
                }
                buttonX += TOGGLE_BUTTON_SIZE + 4;

                // Check mute button
                juce::Rectangle<int> mBtn(buttonX, buttonY, TOGGLE_BUTTON_SIZE, TOGGLE_BUTTON_SIZE);
                if (mBtn.contains(pos.toInt()))
                {
                    toggleAutomationLane(trackId, "mute");
                    return;
                }
            }
        }
        return;
    }

    // Check if click is on an automation point
    juce::String hitTrackId, hitParam, hitPointId;
    if (hitTestAutomationPoint(pos, hitTrackId, hitParam, hitPointId))
    {
        // Start dragging this point
        selectedPoint.set(hitTrackId, hitParam, hitPointId);
        startDraggingPoint(hitTrackId, hitParam, hitPointId);
        repaint();
        return;
    }

    // Check if click is in an automation lane (add new point)
    int trackIndex = getTrackIndexAtY((int)pos.y);
    if (trackIndex >= 0)
    {
        auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
        if (tracksNode.isValid() && trackIndex < tracksNode.getNumChildren())
        {
            auto trackNode = tracksNode.getChild(trackIndex);
            juce::String trackId = trackNode[ProjectState::PROP_ID].toString();
            auto& uiState = getOrCreateUIState(trackId);

            // Check which lane was clicked
            if (uiState.showVolumeLane)
            {
                auto laneArea = getAutomationLaneBounds(trackIndex, "volume");
                if (laneArea.contains(pos))
                {
                    addAutomationPointAt(pos, trackId, "volume");
                    return;
                }
            }

            if (uiState.showPanLane)
            {
                auto laneArea = getAutomationLaneBounds(trackIndex, "pan");
                if (laneArea.contains(pos))
                {
                    addAutomationPointAt(pos, trackId, "pan");
                    return;
                }
            }

            if (uiState.showMuteLane)
            {
                auto laneArea = getAutomationLaneBounds(trackIndex, "mute");
                if (laneArea.contains(pos))
                {
                    addAutomationPointAt(pos, trackId, "mute");
                    return;
                }
            }
        }
    }

    // Clear selection if clicked elsewhere
    selectedPoint.clear();
    repaint();
}

void ArrangerComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isDragging)
    {
        updateDraggedPoint(event.position);
        repaint();
    }
}

void ArrangerComponent::mouseUp(const juce::MouseEvent& event)
{
    if (isDragging)
    {
        finishDraggingPoint();
    }
}

void ArrangerComponent::mouseMove(const juce::MouseEvent& event)
{
    // Update hover state
    juce::String oldHoverTrackId = hoverTrackId;
    juce::String oldHoverParam = hoverParam;
    juce::String oldHoverPointId = hoverPointId;

    hitTestAutomationPoint(event.position, hoverTrackId, hoverParam, hoverPointId);

    // Repaint if hover changed
    if (hoverTrackId != oldHoverTrackId || hoverParam != oldHoverParam || hoverPointId != oldHoverPointId)
    {
        repaint();
    }
}

bool ArrangerComponent::hitTestAutomationPoint(const juce::Point<float>& pos,
                                                juce::String& outTrackId,
                                                juce::String& outParam,
                                                juce::String& outPointId)
{
    outTrackId = "";
    outParam = "";
    outPointId = "";

    // Check all visible automation lanes
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return false;

    int numTracks = tracksNode.getNumChildren();

    for (int i = 0; i < numTracks; ++i)
    {
        auto trackNode = tracksNode.getChild(i);
        juce::String trackId = trackNode[ProjectState::PROP_ID].toString();
        auto& uiState = getOrCreateUIState(trackId);

        // Check each visible lane
        std::vector<juce::String> paramsToCheck;
        if (uiState.showVolumeLane) paramsToCheck.push_back("volume");
        if (uiState.showPanLane) paramsToCheck.push_back("pan");
        if (uiState.showMuteLane) paramsToCheck.push_back("mute");

        for (const auto& param : paramsToCheck)
        {
            auto laneArea = getAutomationLaneBounds(i, param);
            if (!laneArea.contains(pos))
                continue;

            auto& curve = getOrCreateCurveView(trackId, param);
            if (curve.needsRebuild)
            {
                rebuildAutomationCurve(trackId, param);
                curve.needsRebuild = false;
            }

            // Check each point
            for (const auto& point : curve.points)
            {
                float x = beatToX(point.timeBeats);
                float y = valueToY(point.value, param, laneArea);

                float dx = pos.x - x;
                float dy = pos.y - y;
                float distSq = dx * dx + dy * dy;

                if (distSq <= HIT_TEST_RADIUS * HIT_TEST_RADIUS)
                {
                    outTrackId = trackId;
                    outParam = param;
                    outPointId = point.id;
                    return true;
                }
            }
        }
    }

    return false;
}

void ArrangerComponent::addAutomationPointAt(const juce::Point<float>& pos,
                                              const juce::String& trackId,
                                              const juce::String& param)
{
    // Find which track index this is
    int trackIndex = -1;
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (tracksNode.isValid())
    {
        for (int i = 0; i < tracksNode.getNumChildren(); ++i)
        {
            auto trackNode = tracksNode.getChild(i);
            if (trackNode[ProjectState::PROP_ID].toString() == trackId)
            {
                trackIndex = i;
                break;
            }
        }
    }

    if (trackIndex < 0)
        return;

    auto laneArea = getAutomationLaneBounds(trackIndex, param);

    double timeBeats = snapToGrid(xToBeat(pos.x));
    double value = yToValue(pos.y, param, laneArea);

    // Clamp value
    if (param == "volume" || param == "mute")
        value = juce::jlimit(0.0, 1.0, value);
    else if (param == "pan")
        value = juce::jlimit(-1.0, 1.0, value);

    // Add via ProjectState API
    juce::String pointId = projectState.addAutomationPoint(trackId, param, timeBeats, value,
                                                            "Automation: Add " + param + " point");

    // Select the new point
    selectedPoint.set(trackId, param, pointId);

    // Mark curve as needing rebuild
    auto* curve = findCurveView(trackId, param);
    if (curve)
        curve->needsRebuild = true;

    repaint();
}

void ArrangerComponent::startDraggingPoint(const juce::String& trackId,
                                            const juce::String& param,
                                            const juce::String& pointId)
{
    isDragging = true;
    dragTrackId = trackId;
    dragParam = param;
    dragPointId = pointId;

    // Find the point to get its current values
    auto& curve = getOrCreateCurveView(trackId, param);
    if (curve.needsRebuild)
    {
        rebuildAutomationCurve(trackId, param);
        curve.needsRebuild = false;
    }

    for (const auto& point : curve.points)
    {
        if (point.id == pointId)
        {
            dragStartTimeBeats = point.timeBeats;
            dragStartValue = point.value;
            break;
        }
    }
}

void ArrangerComponent::updateDraggedPoint(const juce::Point<float>& pos)
{
    // This is a preview during drag - we don't update ProjectState until mouseUp
    // For now, just update the visual position in the cache

    // Find track index
    int trackIndex = -1;
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (tracksNode.isValid())
    {
        for (int i = 0; i < tracksNode.getNumChildren(); ++i)
        {
            auto trackNode = tracksNode.getChild(i);
            if (trackNode[ProjectState::PROP_ID].toString() == dragTrackId)
            {
                trackIndex = i;
                break;
            }
        }
    }

    if (trackIndex < 0)
        return;

    auto laneArea = getAutomationLaneBounds(trackIndex, dragParam);

    double newTimeBeats = snapToGrid(xToBeat(pos.x));
    double newValue = yToValue(pos.y, dragParam, laneArea);

    // Clamp value
    if (dragParam == "volume" || dragParam == "mute")
        newValue = juce::jlimit(0.0, 1.0, newValue);
    else if (dragParam == "pan")
        newValue = juce::jlimit(-1.0, 1.0, newValue);

    // Update the point in the cache for immediate visual feedback
    auto* curve = findCurveView(dragTrackId, dragParam);
    if (curve)
    {
        for (auto& point : curve->points)
        {
            if (point.id == dragPointId)
            {
                point.timeBeats = newTimeBeats;
                point.value = newValue;
                break;
            }
        }

        // Re-sort
        std::sort(curve->points.begin(), curve->points.end(),
                  [](const AutomationPointView& a, const AutomationPointView& b) {
                      return a.timeBeats < b.timeBeats;
                  });
    }
}

void ArrangerComponent::finishDraggingPoint()
{
    if (!isDragging)
        return;

    // Find track index
    int trackIndex = -1;
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (tracksNode.isValid())
    {
        for (int i = 0; i < tracksNode.getNumChildren(); ++i)
        {
            auto trackNode = tracksNode.getChild(i);
            if (trackNode[ProjectState::PROP_ID].toString() == dragTrackId)
            {
                trackIndex = i;
                break;
            }
        }
    }

    if (trackIndex >= 0)
    {
        // Get final position from cache
        auto* curve = findCurveView(dragTrackId, dragParam);
        if (curve)
        {
            for (const auto& point : curve->points)
            {
                if (point.id == dragPointId)
                {
                    // Only update if actually changed
                    if (point.timeBeats != dragStartTimeBeats || point.value != dragStartValue)
                    {
                        projectState.moveAutomationPoint(dragTrackId, dragParam, dragPointId,
                                                         point.timeBeats, point.value,
                                                         "Automation: Move " + dragParam + " point");
                    }
                    break;
                }
            }

            // Mark for rebuild to sync with actual ProjectState
            curve->needsRebuild = true;
        }
    }

    isDragging = false;
    dragTrackId = "";
    dragParam = "";
    dragPointId = "";

    repaint();
}

void ArrangerComponent::deleteSelectedPoint()
{
    if (!selectedPoint.isValid)
        return;

    projectState.deleteAutomationPoint(selectedPoint.trackId, selectedPoint.param, selectedPoint.pointId,
                                        "Automation: Delete " + selectedPoint.param + " point");

    // Mark curve as needing rebuild
    auto* curve = findCurveView(selectedPoint.trackId, selectedPoint.param);
    if (curve)
        curve->needsRebuild = true;

    selectedPoint.clear();
    repaint();
}

//==============================================================================
// ValueTree Listener
//==============================================================================

void ArrangerComponent::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    // Automation point property changed
    if (tree.hasType(ProjectState::ID_POINT))
    {
        // Mark all curves dirty (simpler than tracking which one changed)
        markAllCurvesDirty();
        repaint();
    }
}

void ArrangerComponent::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    // Track added or automation point added
    if (child.hasType(ProjectState::ID_TRACK) || child.hasType(ProjectState::ID_POINT))
    {
        markAllCurvesDirty();
        repaint();
    }
}

void ArrangerComponent::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int)
{
    // Track removed or automation point removed
    if (child.hasType(ProjectState::ID_TRACK) || child.hasType(ProjectState::ID_POINT))
    {
        markAllCurvesDirty();
        repaint();
    }
}

void ArrangerComponent::valueTreeChildOrderChanged(juce::ValueTree&, int, int)
{
    markAllCurvesDirty();
    repaint();
}

void ArrangerComponent::valueTreeParentChanged(juce::ValueTree&)
{
    markAllCurvesDirty();
    repaint();
}

//==============================================================================
// KeyListener
//==============================================================================

bool ArrangerComponent::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    // Delete/Backspace key
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        deleteSelectedPoint();
        return true;
    }

    return false;
}
