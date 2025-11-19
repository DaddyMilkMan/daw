/**
 * @file TempoLaneComponent.cpp
 * @brief Tempo lane implementation
 */

#include "../include/TempoLaneComponent.h"

//==============================================================================
TempoLaneComponent::TempoLaneComponent(ProjectState& state)
    : projectState(state)
{
    setWantsKeyboardFocus(true);

    // Listen to tempo map changes
    projectState.getState().addListener(this);

    DBG("TempoLaneComponent: Constructor");
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

    // Label
    g.setColour(juce::Colours::lightgrey);
    g.setFont(12.0f);
    g.drawText("TEMPO", bounds.removeFromLeft(80).reduced(5), juce::Justification::centredLeft);

    // Draw tempo points
    auto tempoPoints = projectState.getTempoPoints();

    for (int i = 0; i < tempoPoints.size(); ++i)
    {
        auto pointObj = tempoPoints[i].getDynamicObject();
        if (pointObj == nullptr)
            continue;

        juce::String id = pointObj->getProperty(ProjectState::PROP_ID).toString();
        double timeBeats = pointObj->getProperty(ProjectState::PROP_TIME_BEATS);
        double bpm = pointObj->getProperty(ProjectState::PROP_BPM);

        bool selected = (id == selectedPointId);
        drawTempoPoint(g, timeBeats, bpm, selected);

        // Draw line to next point
        if (i + 1 < tempoPoints.size())
        {
            auto nextPointObj = tempoPoints[i + 1].getDynamicObject();
            if (nextPointObj != nullptr)
            {
                double nextTimeBeats = nextPointObj->getProperty(ProjectState::PROP_TIME_BEATS);
                float x1 = beatsToX(timeBeats);
                float y1 = bpmToY(bpm);
                float x2 = beatsToX(nextTimeBeats);
                float y2 = bpmToY(bpm);  // Step, not linear

                g.setColour(juce::Colours::cyan.withAlpha(0.5f));
                g.drawLine(x1, y1, x2, y2, 2.0f);
            }
        }
    }
}

void TempoLaneComponent::resized()
{
    // Nothing special for MVP
}

void TempoLaneComponent::mouseDown(const juce::MouseEvent& event)
{
    auto x = event.position.x;
    auto y = event.position.y;

    // Find point at click position
    juce::String pointId = findPointAt(x, y);

    if (pointId.isNotEmpty())
    {
        selectedPointId = pointId;
        draggingPointId = pointId;
        dragStart = event.position;

        // Store original values
        auto points = projectState.getTempoPoints();
        for (auto& point : points)
        {
            auto obj = point.getDynamicObject();
            if (obj && obj->getProperty(ProjectState::PROP_ID).toString() == pointId)
            {
                dragStartBeats = obj->getProperty(ProjectState::PROP_TIME_BEATS);
                dragStartBpm = obj->getProperty(ProjectState::PROP_BPM);
                break;
            }
        }

        repaint();
    }
    else
    {
        selectedPointId = juce::String();
        repaint();
    }
}

void TempoLaneComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (draggingPointId.isEmpty())
        return;

    auto delta = event.position - dragStart;

    // Calculate new position
    double newBeats = dragStartBeats + (delta.x / getWidth()) * (viewEndBeats - viewStartBeats);
    double newBpm = dragStartBpm - (delta.y / getHeight()) * (maxBpm - minBpm);

    // Clamp
    newBeats = juce::jmax(0.0, newBeats);
    newBpm = juce::jlimit(minBpm, maxBpm, newBpm);

    // Update point
    projectState.moveTempoPoint(draggingPointId, newBeats, newBpm, "Move Tempo Point");
}

void TempoLaneComponent::mouseUp(const juce::MouseEvent& /* event */)
{
    draggingPointId = juce::String();
}

void TempoLaneComponent::mouseDoubleClick(const juce::MouseEvent& event)
{
    // Add new tempo point at click position
    double beats = xToBeats(event.position.x);
    double bpm = 120.0;  // Default BPM

    projectState.addTempoPoint(beats, bpm, "Add Tempo Point");

    DBG("TempoLaneComponent: Added tempo point at " + juce::String(beats) + " beats");
}

bool TempoLaneComponent::keyPressed(const juce::KeyPress& key)
{
    if (selectedPointId.isEmpty())
        return false;

    // Delete selected point
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        projectState.deleteTempoPoint(selectedPointId, "Delete Tempo Point");
        selectedPointId = juce::String();
        repaint();
        return true;
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
    // Not relevant
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
    const float hitRadius = 10.0f;

    auto points = projectState.getTempoPoints();

    for (auto& point : points)
    {
        auto obj = point.getDynamicObject();
        if (obj == nullptr)
            continue;

        double timeBeats = obj->getProperty(ProjectState::PROP_TIME_BEATS);
        double bpm = obj->getProperty(ProjectState::PROP_BPM);

        float px = beatsToX(timeBeats);
        float py = bpmToY(bpm);

        float dx = x - px;
        float dy = y - py;
        float distSq = dx * dx + dy * dy;

        if (distSq < hitRadius * hitRadius)
        {
            return obj->getProperty(ProjectState::PROP_ID).toString();
        }
    }

    return juce::String();
}

void TempoLaneComponent::drawTempoPoint(juce::Graphics& g, double timeBeats, double bpm, bool selected)
{
    float x = beatsToX(timeBeats);
    float y = bpmToY(bpm);

    const float radius = selected ? 6.0f : 4.0f;

    // Draw point
    g.setColour(selected ? juce::Colours::yellow : juce::Colours::cyan);
    g.fillEllipse(x - radius, y - radius, radius * 2, radius * 2);

    // Draw BPM label
    g.setFont(10.0f);
    g.setColour(juce::Colours::white);
    juce::String label = juce::String(static_cast<int>(bpm));
    g.drawText(label, x - 20, y - 20, 40, 15, juce::Justification::centred);
}
