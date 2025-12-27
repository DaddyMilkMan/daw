/*
  ==============================================================================

    HelpViewPanel.h
    Created: 2025-12-25
    Author:  Zenith DAW

    Info View panel similar to Ableton Live's, displaying context-sensitive help.

  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"

namespace zenith {

class HelpViewPanel : public SkiaComponent {
public:
    HelpViewPanel();
    ~HelpViewPanel() override;

    void drawSkia(SkCanvas* canvas) override;
    void setContent(const juce::String& title, const juce::String& description);

private:
    juce::String title_ = "Info View";
    juce::String description_ = "Hover over controls to see details here.";
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelpViewPanel)
};

} // namespace zenith
