/**
 * @file TempoLaneComponent.cpp
 * @brief Tempo lane implementation
 */

#include "../../include/ui/TempoLaneComponent.h"

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
    g.drawText("TEMPO (Not Implemented)", bounds, juce::Justification::centred);
}

void TempoLaneComponent::resized()
{
}

void TempoLaneComponent::mouseDown(const juce::MouseEvent& /* event */)
{
}

void TempoLaneComponent::mouseDrag(const juce::MouseEvent& /* event */)
{
}

void TempoLaneComponent::mouseUp(const juce::MouseEvent& /* event */)
{
}

void TempoLaneComponent::mouseDoubleClick(const juce::MouseEvent& /* event */)
{
}

bool TempoLaneComponent::keyPressed(const juce::KeyPress& /* key */)
{
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

juce::String TempoLaneComponent::findPointAt(float /* x */, float /* y */) const
{
    return juce::String();
}

void TempoLaneComponent::drawTempoPoint(juce::Graphics& /* g */, double /* timeBeats */, double /* bpm */, bool /* selected */)
{
}
