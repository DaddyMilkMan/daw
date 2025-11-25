/**
 * @file SkiaPresetBrowserComponent.h
 * @brief GPU-rendered preset browser with flashy text effects
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

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
