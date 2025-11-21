/**
 * @file SkiaEffectsChainComponent.h
 * @brief GPU-rendered effects chain with Skia
 */

#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA

namespace zenith {

class SkiaEffectsChainComponent : public juce::Component
{
public:
    SkiaEffectsChainComponent();
    ~SkiaEffectsChainComponent() override = default;

    void addEffect(const juce::String& effectName);
    void removeEffect(int index);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    std::vector<juce::String> effects_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaEffectsChainComponent)
};

}

#endif
