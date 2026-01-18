/*
  ==============================================================================

    GlobalSettingsPanel.h
    Created: 2026-01-18
    Author:  Zenith DAW Team

    Placeholder for GlobalSettingsPanel - stub implementation

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace zenith {

class GlobalSettingsPanel : public juce::Component {
public:
    GlobalSettingsPanel();
    ~GlobalSettingsPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
};

} // namespace zenith
