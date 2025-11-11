/*
  ==============================================================================

    ArrangementComponent.cpp

  ==============================================================================
*/

#include "ArrangementComponent.h"

ArrangementComponent::ArrangementComponent()
{
}

ArrangementComponent::~ArrangementComponent()
{
}

void ArrangementComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId).brighter(0.1f));
    g.setColour(juce::Colours::white);
    g.drawText("Arrangement View", getLocalBounds(), juce::Justification::centred);
}

void ArrangementComponent::resized()
{
}
