/*
  ==============================================================================

    TransportComponent.h
    Created: 2025-11-11
    Author:  Vexel DAW

    Transport controls (Play, Stop, Record, Tempo, etc.)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
class TransportComponent : public juce::Component
{
public:
    TransportComponent();
    ~TransportComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // TODO: Implement transport controls
    juce::TextButton playButton{"Play"};
    juce::TextButton stopButton{"Stop"};
    juce::TextButton recordButton{"Rec"};
    juce::Slider tempoSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportComponent)
};
