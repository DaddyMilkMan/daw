/*
  ==============================================================================

    MixerComponent.cpp

  ==============================================================================
*/

#include "MixerComponent.h"

MixerComponent::MixerComponent()
{
}

MixerComponent::~MixerComponent()
{
}

void MixerComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::white);
    g.drawText("Mixer", getLocalBounds(), juce::Justification::centred);
}

void MixerComponent::resized()
{
}
