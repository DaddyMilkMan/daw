/**
 * @file SkiaInstrumentBrowserComponent.h
 * @brief GPU-rendered instrument browser
 */

#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA

namespace zenith {

class SkiaInstrumentBrowserComponent : public juce::Component
{
public:
    SkiaInstrumentBrowserComponent();
    ~SkiaInstrumentBrowserComponent() override = default;

    void addInstrument(const juce::String& name);
    void selectInstrument(int index);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    std::vector<juce::String> instruments_;
    int selectedIndex_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaInstrumentBrowserComponent)
};

}

#endif
