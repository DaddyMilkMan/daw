/**
 * @file MarkerLaneComponent.cpp
 * @brief Marker lane implementation
 */

#include "../../include/ui/MarkerLaneComponent.h"
// Force rebuild

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
    g.drawText("MARKERS (Not Implemented)", bounds, juce::Justification::centred);
}

void MarkerLaneComponent::resized()
{
}

void MarkerLaneComponent::mouseDown(const juce::MouseEvent& /* event */)
{
}

void MarkerLaneComponent::mouseDrag(const juce::MouseEvent& /* event */)
{
}

void MarkerLaneComponent::mouseUp(const juce::MouseEvent& /* event */)
{
}

void MarkerLaneComponent::mouseDoubleClick(const juce::MouseEvent& /* event */)
{
}

bool MarkerLaneComponent::keyPressed(const juce::KeyPress& /* key */)
{
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

juce::String MarkerLaneComponent::findMarkerAt(float /* x */, float /* y */) const
{
    return juce::String();
}

void MarkerLaneComponent::drawMarker(juce::Graphics& /* g */, double /* timeBeats */, const juce::String& /* name */, bool /* selected */)
{
}

juce::String MarkerLaneComponent::generateMarkerName() const
{
    return "Marker";
}

void MarkerLaneComponent::showRenameDialog(const juce::String& /* markerId */)
{
}
