/*
  ==============================================================================

    BrowserComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class BrowserComponent : public juce::Component
{
public:
    BrowserComponent();
    ~BrowserComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserComponent)
};
