/**
 * @file SkiaEffectsChainComponent.cpp
 */

#include "SkiaEffectsChainComponent.h"

#ifdef ZENITH_USE_SKIA

namespace zenith {

SkiaEffectsChainComponent::SkiaEffectsChainComponent()
{
    setSize(400, 200);
}

void SkiaEffectsChainComponent::addEffect(const juce::String& effectName)
{
    effects_.push_back(effectName);
    repaint();
}

void SkiaEffectsChainComponent::removeEffect(int index)
{
    if (index >= 0 && index < static_cast<int>(effects_.size()))
    {
        effects_.erase(effects_.begin() + index);
        repaint();
    }
}

void SkiaEffectsChainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    g.setColour(juce::Colours::white);
    g.setFont(14);
    g.drawText("Effects Chain", 10, 10, 200, 25, juce::Justification::left);

    int y = 40;
    for (const auto& effect : effects_)
    {
        g.setColour(juce::Colours::darkgrey);
        g.fillRect(10, y, getWidth() - 20, 30);
        g.setColour(juce::Colours::white);
        g.drawRect(10, y, getWidth() - 20, 30);
        g.drawText(effect, 15, y + 5, 200, 20, juce::Justification::left);
        y += 35;
    }
}

void SkiaEffectsChainComponent::resized()
{
    // Layout
}

}

#endif
