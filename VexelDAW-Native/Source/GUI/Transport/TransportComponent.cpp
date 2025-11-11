/*
  ==============================================================================

    TransportComponent.cpp
    Created: 2025-11-11
    Author:  Vexel DAW

  ==============================================================================
*/

#include "TransportComponent.h"

TransportComponent::TransportComponent()
{
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(recordButton);
    addAndMakeVisible(tempoSlider);

    tempoSlider.setRange(20.0, 300.0, 0.1);
    tempoSlider.setValue(120.0);
    tempoSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
}

TransportComponent::~TransportComponent()
{
}

void TransportComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId).darker());
}

void TransportComponent::resized()
{
    auto area = getLocalBounds().reduced(10);

    playButton.setBounds(area.removeFromLeft(60));
    area.removeFromLeft(5);
    stopButton.setBounds(area.removeFromLeft(60));
    area.removeFromLeft(5);
    recordButton.setBounds(area.removeFromLeft(60));
    area.removeFromLeft(20);
    tempoSlider.setBounds(area.removeFromLeft(150));
}
