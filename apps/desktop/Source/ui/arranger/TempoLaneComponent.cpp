/**
 * @file TempoLaneComponent.cpp
 * @brief Tempo lane implementation - FULLY IMPLEMENTED
 * 
 * Allows visual editing of tempo changes in the timeline.
 * Features:
 * - Display tempo curve with interpolation
 * - Create tempo points (double-click)
 * - Drag tempo points (horizontal for time, vertical for BPM)
 * - Delete tempo points (Delete key)
 * - Sync with ProjectState tempo map
 */

#include "TempoLaneComponent.h"

using namespace zenith;

//==============================================================================
TempoLaneComponent::TempoLaneComponent(ProjectState& state)
    : projectState(state)
{
    setWantsKeyboardFocus(true);

    // Listen to tempo map changes
    projectState.getState().addListener(this);

    DBG("TempoLaneComponent: Constructor - FULLY IMPLEMENTED");
}

TempoLaneComponent::~TempoLaneComponent()
{
    projectState.getState().removeListener(this);
    DBG("TempoLaneComponent: Destructor");
}

//==============================================================================
// Component Interface
//==============================================================================

void TempoLaneComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Border
    g.setColour(juce::Colours::black);
    g.drawRect(bounds, 1);

    // Draw grid lines for BPM
    drawGrid(g);

    // Draw tempo curve
    drawTempoCurve(g);

    // Draw tempo points
    drawTempoPoints(g);

    // Draw hovered point highlight
    if (hoveredPointId.isNotEmpty())
    {
        auto tempoMap = projectState.getTempoMap();
        for (auto point : tempoMap)
        {
            if (point[ProjectState::PROP_ID].toString() == hoveredPointId)
            {
                double timeBeats = point[ProjectState::PROP_TIME_BEATS];
                double bpm = point[ProjectState::PROP_BPM];
                float x = beatsToX(timeBeats);
                float y = bpmToY(bpm);

                g.setColour(juce::Colours::yellow.withAlpha(0.3f));
                g.fillEllipse(x - 8, y - 8, 16, 16);
                break;
            }
        }
    }
}

void TempoLaneComponent::resized()
{
}

void TempoLaneComponent::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
        return;

    auto clickPos = event.getPosition().toFloat();

    // Try to select a point
    selectedPointId = findPointAt(clickPos.x, clickPos.y);

    if (selectedPointId.isNotEmpty())
    {
        // Start dragging
        isDraggingPoint = true;
        dragStartX = clickPos.x;
        dragStartY = clickPos.y;
        repaint();
    }
}

void TempoLaneComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDraggingPoint || selectedPointId.isEmpty())
        return;

    auto currentPos = event.getPosition().toFloat();

    // Calculate new position
    double newBeats = xToBeats(currentPos.x);
    double newBpm = yToBpm(currentPos.y);

    // Clamp values
    newBeats = juce::jmax(0.0, newBeats);
    newBpm = juce::jlimit(minBpm, maxBpm, newBpm);

    // Update ProjectState
    auto tempoMap = projectState.getTempoMap();
    for (auto point : tempoMap)
    {
        if (point[ProjectState::PROP_ID].toString() == selectedPointId)
        {
            point.setProperty(ProjectState::PROP_TIME_BEATS, newBeats, &projectState.getUndoManager());
            point.setProperty(ProjectState::PROP_BPM, newBpm, &projectState.getUndoManager());
            break;
        }
    }

    repaint();
}

void TempoLaneComponent::mouseUp(const juce::MouseEvent& /* event */)
{
    isDraggingPoint = false;
}

void TempoLaneComponent::mouseDoubleClick(const juce::MouseEvent& event)
{
    // Create new tempo point
    auto clickPos = event.getPosition().toFloat();
    double timeBeats = xToBeats(clickPos.x);
    double bpm = yToBpm(clickPos.y);

    // Clamp values
    timeBeats = juce::jmax(0.0, timeBeats);
    bpm = juce::jlimit(minBpm, maxBpm, bpm);

    // Add to ProjectState
    projectState.addTempoChange(timeBeats, bpm, "Add tempo point");

    repaint();
}

void TempoLaneComponent::mouseMove(const juce::MouseEvent& event)
{
    auto currentPos = event.getPosition().toFloat();
    juce::String newHoveredId = findPointAt(currentPos.x, currentPos.y);

    if (newHoveredId != hoveredPointId)
    {
        hoveredPointId = newHoveredId;
        repaint();
    }
}

void TempoLaneComponent::mouseExit(const juce::MouseEvent& /* event */)
{
    if (hoveredPointId.isNotEmpty())
    {
        hoveredPointId = juce::String();
        repaint();
    }
}

bool TempoLaneComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        if (selectedPointId.isNotEmpty())
        {
            // Delete selected point
            auto tempoMap = projectState.getTempoMap();
            for (int i = 0; i < tempoMap.getNumChildren(); ++i)
            {
                auto point = tempoMap.getChild(i);
                if (point[ProjectState::PROP_ID].toString() == selectedPointId)
                {
                    tempoMap.removeChild(i, &projectState.getUndoManager());
                    selectedPointId = juce::String();
                    repaint();
                    return true;
                }
            }
        }
    }

    return false;
}

//==============================================================================
// ValueTree::Listener
//==============================================================================

void TempoLaneComponent::valueTreePropertyChanged(juce::ValueTree& /* tree */, const juce::Identifier& /* property */)
{
    repaint();
}

void TempoLaneComponent::valueTreeChildAdded(juce::ValueTree& /* parent */, juce::ValueTree& /* child */)
{
    repaint();
}

void TempoLaneComponent::valueTreeChildRemoved(juce::ValueTree& /* parent */, juce::ValueTree& /* child */, int /* index */)
{
    repaint();
}

void TempoLaneComponent::valueTreeChildOrderChanged(juce::ValueTree& /* parent */, int /* oldIndex */, int /* newIndex */)
{
    repaint();
}

void TempoLaneComponent::valueTreeParentChanged(juce::ValueTree& /* tree */)
{
}

//==============================================================================
// Helper Methods
//==============================================================================

double TempoLaneComponent::xToBeats(float x) const
{
    float normalized = x / getWidth();
    return viewStartBeats + normalized * (viewEndBeats - viewStartBeats);
}

float TempoLaneComponent::beatsToX(double beats) const
{
    double normalized = (beats - viewStartBeats) / (viewEndBeats - viewStartBeats);
    return static_cast<float>(normalized * getWidth());
}

double TempoLaneComponent::yToBpm(float y) const
{
    float normalized = y / getHeight();
    return maxBpm - normalized * (maxBpm - minBpm);
}

float TempoLaneComponent::bpmToY(double bpm) const
{
    double normalized = (maxBpm - bpm) / (maxBpm - minBpm);
    return static_cast<float>(normalized * getHeight());
}

juce::String TempoLaneComponent::findPointAt(float x, float y) const
{
    const float hitRadius = 8.0f;
    auto tempoMap = projectState.getTempoMap();

    for (auto point : tempoMap)
    {
        double timeBeats = point[ProjectState::PROP_TIME_BEATS];
        double bpm = point[ProjectState::PROP_BPM];

        float px = beatsToX(timeBeats);
        float py = bpmToY(bpm);

        float distance = std::sqrt((x - px) * (x - px) + (y - py) * (y - py));
        if (distance <= hitRadius)
        {
            return point[ProjectState::PROP_ID].toString();
        }
    }

    return juce::String();
}

void TempoLaneComponent::drawGrid(juce::Graphics& g) const
{
    auto bounds = getLocalBounds().toFloat();

    // Draw horizontal BPM grid lines
    g.setColour(juce::Colour(0xff3a3a3a));
    const int bpmStep = 20;

    for (int bpm = static_cast<int>(minBpm); bpm <= static_cast<int>(maxBpm); bpm += bpmStep)
    {
        float y = bpmToY(bpm);
        g.drawLine(0, y, bounds.getWidth(), y, 1.0f);

        // Draw BPM label
        g.setColour(juce::Colours::grey);
        g.setFont(10.0f);
        g.drawText(juce::String(bpm) + " BPM", 5, static_cast<int>(y) - 12, 60, 12,
                   juce::Justification::centredLeft);
    }
}

void TempoLaneComponent::drawTempoCurve(juce::Graphics& g) const
{
    auto tempoMap = projectState.getTempoMap();
    if (tempoMap.getNumChildren() == 0)
        return;

    // Draw connecting lines between tempo points
    g.setColour(juce::Colour(0xff4a9eff).withAlpha(0.7f));

    juce::Path curvePath;
    bool firstPoint = true;

    for (auto point : tempoMap)
    {
        double timeBeats = point[ProjectState::PROP_TIME_BEATS];
        double bpm = point[ProjectState::PROP_BPM];

        float x = beatsToX(timeBeats);
        float y = bpmToY(bpm);

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

    g.strokePath(curvePath, juce::PathStrokeType(2.0f));
}

void TempoLaneComponent::drawTempoPoints(juce::Graphics& g) const
{
    auto tempoMap = projectState.getTempoMap();

    for (auto point : tempoMap)
    {
        double timeBeats = point[ProjectState::PROP_TIME_BEATS];
        double bpm = point[ProjectState::PROP_BPM];
        juce::String pointId = point[ProjectState::PROP_ID].toString();

        bool selected = (pointId == selectedPointId);

        drawTempoPoint(g, timeBeats, bpm, selected);
    }
}

void TempoLaneComponent::drawTempoPoint(juce::Graphics& g, double timeBeats, double bpm, bool selected) const
{
    float x = beatsToX(timeBeats);
    float y = bpmToY(bpm);

    // Draw point
    if (selected)
    {
        g.setColour(juce::Colours::orange);
        g.fillEllipse(x - 6, y - 6, 12, 12);
    }
    else
    {
        g.setColour(juce::Colours::white);
        g.fillEllipse(x - 5, y - 5, 10, 10);
    }

    // Draw border
    g.setColour(juce::Colours::black);
    g.drawEllipse(x - 5, y - 5, 10, 10, 1.0f);

    // Draw BPM label
    g.setColour(juce::Colours::white);
    g.setFont(10.0f);
    g.drawText(juce::String(bpm, 1), static_cast<int>(x) - 20, static_cast<int>(y) + 8, 40, 12,
               juce::Justification::centred);
}
