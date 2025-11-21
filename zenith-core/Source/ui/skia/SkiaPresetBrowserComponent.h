/**
 * @file SkiaPresetBrowserComponent.h
 * @brief GPU-rendered preset browser with flashy text effects
 */

#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA

namespace zenith {

class SkiaPresetBrowserComponent : public juce::Component
{
public:
    SkiaPresetBrowserComponent();
    ~SkiaPresetBrowserComponent() override = default;

    void addPreset(const juce::String& name);
    void selectPreset(int index);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    std::vector<juce::String> presets_;
    int selectedIndex_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaPresetBrowserComponent)
};

}

#endif
