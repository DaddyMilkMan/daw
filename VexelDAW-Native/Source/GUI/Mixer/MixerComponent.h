/*
  ==============================================================================

    MixerComponent.h
    Created: 2025-11-11
    Author:  Vexel DAW

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class MixerComponent : public juce::Component
{
public:
    MixerComponent();
    ~MixerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};
