/*
  ==============================================================================
    PresetBrowserComponent.h
    STUB - Original file disabled due to Skia API incompatibilities
    TODO: Refactor to use SkiaComponent base class properly
  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../instruments/ZenithPresetManager.h"

namespace zenith {

// Stub implementation until full refactor
class PresetBrowserComponent : public juce::Component {
public:
    using LoadPresetCallback = std::function<void(const Preset&)>;
    using CaptureStateCallback = std::function<zenith::Preset()>;
    
    PresetBrowserComponent() = default;
    ~PresetBrowserComponent() override = default;
    
    void setEngine(class Engine*) {}
    void setInstrumentId(const juce::String&) {}
    void setLoadPresetCallback(LoadPresetCallback) {}
    void setCaptureStateCallback(CaptureStateCallback) {}
    
    void paint(juce::Graphics& g) override { 
        g.fillAll(juce::Colour(0xFF1a1a2e)); 
        g.setColour(juce::Colours::white);
        g.drawText("PresetBrowser: Pending Skia Refactor", getLocalBounds(), juce::Justification::centred);
    }
    void resized() override {}
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserComponent)
};

} // namespace zenith
