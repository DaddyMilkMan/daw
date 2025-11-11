/*
  ==============================================================================

    ArrangementComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ArrangementComponent : public juce::Component
{
public:
    ArrangementComponent();
    ~ArrangementComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangementComponent)
};
