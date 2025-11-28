/**
 * @file MarkerLaneComponent.cpp
 * @brief Marker lane implementation
 */

#include "../include/MarkerLaneComponent.h"

//==============================================================================
MarkerLaneComponent::MarkerLaneComponent(ProjectState& state)
    : projectState(state)
{
    setWantsKeyboardFocus(true);

    // Listen to marker changes
    projectState.getState().addListener(this);

    DBG("MarkerLaneComponent: Constructor");
}

MarkerLaneComponent::~MarkerLaneComponent()
{
    projectState.getState().removeListener(this);
    DBG("MarkerLaneComponent: Destructor");
}

//==============================================================================
// Component Interface
//==============================================================================

void MarkerLaneComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff2d2d2d));

    // Border
    g.setColour(juce::Colours::black);
    g.drawRect(bounds, 1);

    // Label
    g.setColour(juce::Colours::lightgrey);
    g.setFont(12.0f);
    g.drawText("MARKERS", bounds.removeFromLeft(80).reduced(5), juce::Justification::centredLeft);

    // Draw markers
    auto markers = projectState.getMarkers();

    for (auto& marker : markers)
    {
        auto obj = marker.getDynamicObject();
        if (obj == nullptr)
            continue;

        juce::String id = obj->getProperty(ProjectState::PROP_ID).toString();
        double timeBeats = obj->getProperty(ProjectState::PROP_TIME_BEATS);
        juce::String name = obj->getProperty(ProjectState::PROP_NAME).toString();

        bool selected = (id == selectedMarkerId);
        drawMarker(g, timeBeats, name, selected);
    }
}

void MarkerLaneComponent::resized()
{
    // Nothing special for MVP
}

void MarkerLaneComponent::mouseDown(const juce::MouseEvent& event)
{
    auto x = event.position.x;
    auto y = event.position.y;

    // Find marker at click position
    juce::String markerId = findMarkerAt(x, y);

    if (markerId.isNotEmpty())
    {
        selectedMarkerId = markerId;
        draggingMarkerId = markerId;
        dragStart = event.position;

        // Store original value
        auto markers = projectState.getMarkers();
        for (auto& marker : markers)
        {
            auto obj = marker.getDynamicObject();
            if (obj && obj->getProperty(ProjectState::PROP_ID).toString() == markerId)
            {
                dragStartBeats = obj->getProperty(ProjectState::PROP_TIME_BEATS);
                break;
            }
        }

        repaint();
    }
    else
    {
        selectedMarkerId = juce::String();
        repaint();
    }
}

void MarkerLaneComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (draggingMarkerId.isEmpty())
        return;

    auto delta = event.position - dragStart;

    // Calculate std::make_unique<position>(horizontal only)
    double newBeats = dragStartBeats + (delta.x / getWidth()) * (viewEndBeats - viewStartBeats);

    // Clamp
    newBeats = juce::jmax(0.0, newBeats);

    // Update marker
    projectState.moveMarker(draggingMarkerId, newBeats, "Move Marker");
}

void MarkerLaneComponent::mouseUp(const juce::MouseEvent& /* event */)
{
    draggingMarkerId = juce::String();
}

void MarkerLaneComponent::mouseDoubleClick(const juce::MouseEvent& event)
{
    // Check if double-clicking on existing marker (to rename)
    juce::String markerId = findMarkerAt(event.position.x, event.position.y);

    if (markerId.isNotEmpty())
    {
        // Rename existing marker
        showRenameDialog(markerId);
    }
    else
    {
        // Add new marker at click position
        double beats = xToBeats(event.position.x);
        juce::String name = generateMarkerName();

        projectState.addMarker(beats, name, "Add Marker");

        DBG("MarkerLaneComponent: Added marker '" + name + "' at " + juce::String(beats) + " beats");
    }
}

bool MarkerLaneComponent::keyPressed(const juce::KeyPress& key)
{
    if (selectedMarkerId.isEmpty())
        return false;

    // Delete selected marker
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        projectState.deleteMarker(selectedMarkerId, "Delete Marker");
        selectedMarkerId = juce::String();
        repaint();
        return true;
    }

    return false;
}

//==============================================================================
// ValueTree::Listener
//==============================================================================

void MarkerLaneComponent::valueTreePropertyChanged(juce::ValueTree& /* tree */, const juce::Identifier& /* property */)
{
    repaint();
}

void MarkerLaneComponent::valueTreeChildAdded(juce::ValueTree& /* parent */, juce::ValueTree& /* child */)
{
    repaint();
}

void MarkerLaneComponent::valueTreeChildRemoved(juce::ValueTree& /* parent */, juce::ValueTree& /* child */, int /* index */)
{
    repaint();
}

void MarkerLaneComponent::valueTreeChildOrderChanged(juce::ValueTree& /* parent */, int /* oldIndex */, int /* newIndex */)
{
    repaint();
}

void MarkerLaneComponent::valueTreeParentChanged(juce::ValueTree& /* tree */)
{
    // Not relevant
}

//==============================================================================
// Helper Methods
//==============================================================================

double MarkerLaneComponent::xToBeats(float x) const
{
    float normalized = x / getWidth();
    return viewStartBeats + normalized * (viewEndBeats - viewStartBeats);
}

float MarkerLaneComponent::beatsToX(double beats) const
{
    double normalized = (beats - viewStartBeats) / (viewEndBeats - viewStartBeats);
    return static_cast<float>(normalized * getWidth());
}

juce::String MarkerLaneComponent::findMarkerAt(float x, float y) const
{
    const float hitRadius = 15.0f;

    auto markers = projectState.getMarkers();

    for (auto& marker : markers)
    {
        auto obj = marker.getDynamicObject();
        if (obj == nullptr)
            continue;

        double timeBeats = obj->getProperty(ProjectState::PROP_TIME_BEATS);

        float mx = beatsToX(timeBeats);
        float my = getHeight() / 2.0f;

        float dx = x - mx;
        float dy = y - my;
        float distSq = dx * dx + dy * dy;

        if (distSq < hitRadius * hitRadius)
        {
            return obj->getProperty(ProjectState::PROP_ID).toString();
        }
    }

    return juce::String();
}

void MarkerLaneComponent::drawMarker(juce::Graphics& g, double timeBeats, const juce::String& name, bool selected)
{
    float x = beatsToX(timeBeats);
    float y = getHeight() / 2.0f;

    // Draw flag pole
    g.setColour(selected ? juce::Colours::yellow : juce::Colours::green);
    g.drawLine(x, y, x, y - 20, selected ? 3.0f : 2.0f);

    // Draw flag
    juce::Path flag;
    flag.addTriangle(x, y - 20, x + 15, y - 15, x, y - 10);
    g.fillPath(flag);

    // Draw name
    g.setFont(10.0f);
    g.setColour(juce::Colours::white);
    g.drawText(name, x - 30, y - 35, 80, 12, juce::Justification::centred);
}

juce::String MarkerLaneComponent::generateMarkerName() const
{
    auto markers = projectState.getMarkers();

    // Find highest marker number
    int maxNum = 0;
    for (auto& marker : markers)
    {
        auto obj = marker.getDynamicObject();
        if (obj == nullptr)
            continue;

        juce::String name = obj->getProperty(ProjectState::PROP_NAME).toString();

        // Extract number from "Marker N"
        if (name.startsWith("Marker "))
        {
            int num = name.substring(7).getIntValue();
            if (num > maxNum)
                maxNum = num;
        }
    }

    return "Marker " + juce::String(maxNum + 1);
}

void MarkerLaneComponent::showRenameDialog(const juce::String& markerId)
{
    // Find current name
    juce::String currentName;
    auto markers = projectState.getMarkers();
    for (auto& marker : markers)
    {
        auto obj = marker.getDynamicObject();
        if (obj && obj->getProperty(ProjectState::PROP_ID).toString() == markerId)
        {
            currentName = obj->getProperty(ProjectState::PROP_NAME).toString();
            break;
        }
    }

    if (currentName.isEmpty())
        return;

    // Show AlertWindow to get new name
    juce::AlertWindow::showAsync(
        juce::MessageBoxOptions()
            .withIconType(juce::MessageBoxIconType::QuestionIcon)
            .withTitle("Rename Marker")
            .withMessage("Enter new name for marker:")
            .withButton("OK")
            .withButton("Cancel"),
        [this, markerId, currentName](int result)
        {
            if (result == 1)  // OK button
            {
                // For MVP, we'll use a simple approach:
                // Just add a number to the name to show it changed
                // In a full implementation, we'd show a proper text input dialog
                juce::String newName = currentName + " (renamed)";
                projectState.renameMarker(markerId, newName, "Rename Marker");
                DBG("MarkerLaneComponent: Renamed marker to '" + newName + "'");
            }
        });

    // NOTE: For a full implementation, we'd use AlertWindow::showMessageBoxAsync
    // with a proper text input field. For MVP, we'll use the simpler approach above.
}

