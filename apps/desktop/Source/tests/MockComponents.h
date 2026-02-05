#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

class MockComponent : public juce::Component {
public:
    MockComponent();

    void paint(juce::Graphics& g) override;
    void resized() override;
};