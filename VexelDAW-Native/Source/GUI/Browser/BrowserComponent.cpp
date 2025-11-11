/*
  ==============================================================================

    BrowserComponent.cpp

  ==============================================================================
*/

#include "BrowserComponent.h"

BrowserComponent::BrowserComponent()
{
}

BrowserComponent::~BrowserComponent()
{
}

void BrowserComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId).darker(0.2f));
    g.setColour(juce::Colours::white);
    g.drawText("File Browser", getLocalBounds(), juce::Justification::centred);
}

void BrowserComponent::resized()
{
}
