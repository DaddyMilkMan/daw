/**
 * @file AutomationLaneComponent.cpp
 * @brief Implementation of AutomationLaneComponent
 *
 * Phase U5: Automation Lanes UI
 */

#include "AutomationLaneComponent.h"

//==============================================================================
// Constructor / Destructor
//==============================================================================

AutomationLaneComponent::AutomationLaneComponent(ProjectState& state,
                                                 const juce::String& trackId_,
                                                 const juce::String& paramId_)
    : projectState(state),
      trackId(trackId_),
      paramId(paramId_)
{
    // Get envelope node (creates if doesn't exist)
    envelopeNode = projectState.getOrCreateAutomationEnvelope(trackId, paramId);
    envelopeNode.addListener(this);

    // Set default parameter info based on parameter type
    if (paramId == "volume")
        paramInfo = getDefaultVolumeInfo();
    else if (paramId == "pan")
    {
        paramInfo.displayName = "Pan";
        paramInfo.minValue = -1.0;
        paramInfo.maxValue = 1.0;
        paramInfo.units = "";
        paramInfo.valueToString = [](double v) { return juce::String(v, 2); };
    }
    else if (paramId == "mute")
    {
        paramInfo.displayName = "Mute";
        paramInfo.minValue = 0.0;
        paramInfo.maxValue = 1.0;
        paramInfo.units = "";
        paramInfo.valueToString = [](double v) { return v >= 0.5 ? "ON" : "OFF"; };
    }
}

AutomationLaneComponent::~AutomationLaneComponent()
{
    if (envelopeNode.isValid())
        envelopeNode.removeListener(this);
}

//==============================================================================
// Visual Settings
//==============================================================================

void AutomationLaneComponent::setPixelsPerBeat(double ppb)
{
    pixelsPerBeat = juce::jmax(1.0, ppb);
    repaint();
}

void AutomationLaneComponent::setScrollOffsetBeats(double offset)
{
    scrollOffsetBeats = offset;
    repaint();
}

void AutomationLaneComponent::setGridSnapEnabled(bool enabled, double gridBeats_)
{
    gridSnapEnabled = enabled;
    gridBeats = juce::jmax(0.25, gridBeats_);
}

//==============================================================================
// Parameter Info
//==============================================================================

void AutomationLaneComponent::setParameterInfo(const ParamInfo& info)
{
    paramInfo = info;
    repaint();
}

AutomationLaneComponent::ParamInfo AutomationLaneComponent::getDefaultVolumeInfo()
{
    ParamInfo info;
    info.displayName = "Volume";
    info.minValue = 0.0;
    info.maxValue = 1.0;
    info.units = "";
    info.valueToString = [](double v)
    {
        // Convert to percentage
        return juce::String(static_cast<int>(v * 100)) + "%";
    };
    return info;
}

//==============================================================================
// Component Overrides - Rendering
//==============================================================================

void AutomationLaneComponent::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Draw components
    drawGrid(g);
    drawEnvelopeCurve(g);
    rebuildPointHandles();  // Update hit test cache
    drawControlPoints(g);

    // Draw parameter name
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(14.0f);
    g.drawText(paramInfo.displayName, 5, 5, 100, 20, juce::Justification::centredLeft);
}

void AutomationLaneComponent::resized()
{
    repaint();
}

//==============================================================================
// Drawing Methods
//==============================================================================

void AutomationLaneComponent::drawGrid(juce::Graphics& g)
{
    const int width = getWidth();
    const int height = getHeight();

    g.setColour(juce::Colour(0xff444444));

    // Horizontal lines (value divisions)
    const int numHLines = 5;
    for (int i = 0; i <= numHLines; ++i)
    {
        float y = i * height / static_cast<float>(numHLines);
        g.drawLine(0.0f, y, static_cast<float>(width), y, 0.5f);
    }

    // Vertical lines (beat grid)
    double beatStart = std::floor(scrollOffsetBeats);
    double beatEnd = scrollOffsetBeats + (width / pixelsPerBeat);

    for (double beat = beatStart; beat <= beatEnd; beat += 1.0)
    {
        float x = beatsToPixels(beat);
        if (x >= 0.0f && x <= width)
        {
            // Stronger line every 4 beats (measure)
            if (static_cast<int>(beat) % 4 == 0)
                g.setColour(juce::Colour(0xff666666));
            else
                g.setColour(juce::Colour(0xff444444));

            g.drawLine(x, 0.0f, x, static_cast<float>(height), 0.5f);
        }
    }
}

void AutomationLaneComponent::drawEnvelopeCurve(juce::Graphics& g)
{
    if (!envelopeNode.isValid())
        return;

    const int numPoints = envelopeNode.getNumChildren();
    if (numPoints == 0)
        return;

    // Build path for automation curve
    juce::Path path;
    bool firstPoint = true;

    // Gather all points
    struct PointData
    {
        double timeBeats;
        double value;
        float x;
        float y;
    };
    std::vector<PointData> points;

    for (int i = 0; i < numPoints; ++i)
    {
        auto pointNode = envelopeNode.getChild(i);
        double timeBeats = pointNode.getProperty(ProjectState::PROP_TIME_BEATS, 0.0);
        double value = pointNode.getProperty(ProjectState::PROP_VALUE, 0.0);

        PointData pt;
        pt.timeBeats = timeBeats;
        pt.value = value;
        pt.x = beatsToPixels(timeBeats);
        pt.y = valueToPixelY(value);
        points.push_back(pt);
    }

    // Sort by time (should already be sorted, but just in case)
    std::sort(points.begin(), points.end(),
              [](const PointData& a, const PointData& b) { return a.timeBeats < b.timeBeats; });

    // Draw line from left edge to first point
    if (!points.empty())
    {
        path.startNewSubPath(0.0f, points[0].y);
        path.lineTo(points[0].x, points[0].y);
    }

    // Draw lines between points
    for (size_t i = 0; i < points.size(); ++i)
    {
        if (i == 0)
            path.lineTo(points[i].x, points[i].y);
        else
            path.lineTo(points[i].x, points[i].y);
    }

    // Draw line from last point to right edge (hold value)
    if (!points.empty())
    {
        path.lineTo(static_cast<float>(getWidth()), points.back().y);
    }

    // Draw the path
    g.setColour(juce::Colour(0xff4a9eff));
    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void AutomationLaneComponent::drawControlPoints(juce::Graphics& g)
{
    for (const auto& handle : pointHandles)
    {
        // Check if this point is being dragged
        bool isSelected = (handle.pointId == draggedPointId);

        // Draw circle
        g.setColour(isSelected ? juce::Colour(0xffff9944) : juce::Colour(0xff4a9eff));
        g.fillEllipse(handle.screenPos.x - handle.radius,
                     handle.screenPos.y - handle.radius,
                     handle.radius * 2.0f,
                     handle.radius * 2.0f);

        // Draw outline
        g.setColour(juce::Colours::white);
        g.drawEllipse(handle.screenPos.x - handle.radius,
                     handle.screenPos.y - handle.radius,
                     handle.radius * 2.0f,
                     handle.radius * 2.0f,
                     1.5f);
    }
}

void AutomationLaneComponent::rebuildPointHandles()
{
    pointHandles.clear();

    if (!envelopeNode.isValid())
        return;

    const int numPoints = envelopeNode.getNumChildren();
    for (int i = 0; i < numPoints; ++i)
    {
        auto pointNode = envelopeNode.getChild(i);
        juce::String pointId = pointNode.getProperty(ProjectState::PROP_ID, "");
        double timeBeats = pointNode.getProperty(ProjectState::PROP_TIME_BEATS, 0.0);
        double value = pointNode.getProperty(ProjectState::PROP_VALUE, 0.0);

        PointHandle handle;
        handle.pointId = pointId;
        handle.screenPos = juce::Point<float>(beatsToPixels(timeBeats), valueToPixelY(value));
        pointHandles.push_back(handle);
    }
}

//==============================================================================
// Mouse Interaction
//==============================================================================

void AutomationLaneComponent::mouseDown(const juce::MouseEvent& e)
{
    auto mousePos = e.position;

    // Check if clicking on existing point
    juce::String hitPointId = findPointAtPosition(mousePos);

    if (hitPointId.isNotEmpty())
    {
        // Start dragging existing point
        draggedPointId = hitPointId;
        dragStartMousePos = mousePos;
        isDragging = true;

        // Store original position for potential undo
        if (envelopeNode.isValid())
        {
            for (int i = 0; i < envelopeNode.getNumChildren(); ++i)
            {
                auto pointNode = envelopeNode.getChild(i);
                juce::String pointId = pointNode.getProperty(ProjectState::PROP_ID, "");
                if (pointId == draggedPointId)
                {
                    dragStartTimeBeats = pointNode.getProperty(ProjectState::PROP_TIME_BEATS, 0.0);
                    dragStartValue = pointNode.getProperty(ProjectState::PROP_VALUE, 0.0);
                    break;
                }
            }
        }

        repaint();
    }
    else
    {
        // Add new point at click position
        addPointAt(mousePos);
    }
}

void AutomationLaneComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging || draggedPointId.isEmpty())
        return;

    // Move the dragged point
    movePoint(draggedPointId, e.position);
}

void AutomationLaneComponent::mouseUp(const juce::MouseEvent& e)
{
    if (isDragging)
    {
        isDragging = false;
        draggedPointId = "";
        repaint();
    }
}

void AutomationLaneComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    // Double-click to delete point
    juce::String hitPointId = findPointAtPosition(e.position);
    if (hitPointId.isNotEmpty())
    {
        deletePoint(hitPointId);
    }
}

//==============================================================================
// Coordinate Conversion
//==============================================================================

float AutomationLaneComponent::beatsToPixels(double timeBeats) const
{
    return static_cast<float>((timeBeats - scrollOffsetBeats) * pixelsPerBeat);
}

double AutomationLaneComponent::pixelsToBeats(float pixelX) const
{
    return (pixelX / pixelsPerBeat) + scrollOffsetBeats;
}

float AutomationLaneComponent::valueToPixelY(double value) const
{
    const float height = static_cast<float>(getHeight());
    // Invert Y axis: value 1.0 (max) should be at top (y=0)
    double normalized = (value - paramInfo.minValue) / (paramInfo.maxValue - paramInfo.minValue);
    normalized = juce::jlimit(0.0, 1.0, normalized);
    return height * (1.0f - static_cast<float>(normalized));
}

double AutomationLaneComponent::pixelYToValue(float pixelY) const
{
    const float height = static_cast<float>(getHeight());
    // Invert Y axis: y=0 (top) should be max value
    float normalized = 1.0f - (pixelY / height);
    normalized = juce::jlimit(0.0f, 1.0f, normalized);

    double value = paramInfo.minValue + normalized * (paramInfo.maxValue - paramInfo.minValue);
    return juce::jlimit(paramInfo.minValue, paramInfo.maxValue, value);
}

//==============================================================================
// Hit Testing
//==============================================================================

juce::String AutomationLaneComponent::findPointAtPosition(juce::Point<float> pos) const
{
    for (const auto& handle : pointHandles)
    {
        if (handle.hitTest(pos))
            return handle.pointId;
    }
    return {};
}

//==============================================================================
// Editing Operations
//==============================================================================

void AutomationLaneComponent::addPointAt(juce::Point<float> pos)
{
    // Convert screen position to automation coordinates
    double timeBeats = pixelsToBeats(pos.x);
    double value = pixelYToValue(pos.y);

    // Snap to grid if enabled
    if (gridSnapEnabled)
        timeBeats = quantizeToGrid(timeBeats);

    // Clamp time to positive values
    timeBeats = juce::jmax(0.0, timeBeats);

    // Add point via ProjectState (with undo)
    projectState.addAutomationPoint(trackId, paramId, timeBeats, value, "Add Automation Point");

    // Repaint will happen via ValueTree listener
}

void AutomationLaneComponent::deletePoint(const juce::String& pointId)
{
    if (pointId.isEmpty())
        return;

    // Delete point via ProjectState (with undo)
    projectState.deleteAutomationPoint(trackId, paramId, pointId, "Delete Automation Point");

    // Repaint will happen via ValueTree listener
}

void AutomationLaneComponent::movePoint(const juce::String& pointId, juce::Point<float> newPos)
{
    if (pointId.isEmpty())
        return;

    // Convert screen position to automation coordinates
    double newTimeBeats = pixelsToBeats(newPos.x);
    double newValue = pixelYToValue(newPos.y);

    // Snap to grid if enabled
    if (gridSnapEnabled)
        newTimeBeats = quantizeToGrid(newTimeBeats);

    // Clamp time to positive values
    newTimeBeats = juce::jmax(0.0, newTimeBeats);

    // Move point via ProjectState (with undo)
    projectState.moveAutomationPoint(trackId, paramId, pointId, newTimeBeats, newValue, "Move Automation Point");

    // Repaint will happen via ValueTree listener
}

//==============================================================================
// Grid Snapping
//==============================================================================

double AutomationLaneComponent::quantizeToGrid(double timeBeats) const
{
    if (!gridSnapEnabled || gridBeats <= 0.0)
        return timeBeats;

    return std::round(timeBeats / gridBeats) * gridBeats;
}

//==============================================================================
// ValueTree Listener
//==============================================================================

void AutomationLaneComponent::valueTreePropertyChanged(juce::ValueTree& tree,
                                                       const juce::Identifier& property)
{
    // Any property change in the envelope or its points -> repaint
    repaint();
}

void AutomationLaneComponent::valueTreeChildAdded(juce::ValueTree& parent,
                                                  juce::ValueTree& child)
{
    // Point added -> repaint
    repaint();
}

void AutomationLaneComponent::valueTreeChildRemoved(juce::ValueTree& parent,
                                                    juce::ValueTree& child,
                                                    int index)
{
    // Point removed -> repaint
    repaint();
}

void AutomationLaneComponent::valueTreeChildOrderChanged(juce::ValueTree& parent,
                                                         int oldIndex,
                                                         int newIndex)
{
    // Point order changed (shouldn't happen, but handle it) -> repaint
    repaint();
}

