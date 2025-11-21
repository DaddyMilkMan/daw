/**
 * @file SkiaPresetBrowserComponent.cpp
 */

#include "SkiaPresetBrowserComponent.h"

#ifdef ZENITH_USE_SKIA

namespace zenith {

SkiaPresetBrowserComponent::SkiaPresetBrowserComponent()
{
    setSize(300, 400);
}

void SkiaPresetBrowserComponent::addPreset(const juce::String& name)
{
    presets_.push_back(name);
    repaint();
}

void SkiaPresetBrowserComponent::selectPreset(int index)
{
    if (index >= 0 && index < static_cast<int>(presets_.size()))
    {
        selectedIndex_ = index;
        repaint();
    }
}

void SkiaPresetBrowserComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    g.setColour(juce::Colours::white);
    g.setFont(14);
    g.drawText("Presets", 10, 10, 200, 25, juce::Justification::left);

    int y = 40;
    for (int i = 0; i < static_cast<int>(presets_.size()); i++)
    {
        if (i == selectedIndex_)
        {
            g.setColour(juce::Colour(0, 132, 255)); // Apple blue
            g.fillRect(10, y, getWidth() - 20, 30);
            g.setColour(juce::Colours::white);
        }
        else
        {
            g.setColour(juce::Colours::darkgrey);
            g.fillRect(10, y, getWidth() - 20, 30);
            g.setColour(juce::Colours::white);
        }

        g.drawRect(10, y, getWidth() - 20, 30);
        g.drawText(presets_[i], 15, y + 5, 200, 20, juce::Justification::left);
        y += 35;
    }
}

void SkiaPresetBrowserComponent::resized()
{
}

void SkiaPresetBrowserComponent::mouseDown(const juce::MouseEvent& event)
{
    int itemIndex = (event.y - 40) / 35;
    if (itemIndex >= 0 && itemIndex < static_cast<int>(presets_.size()))
    {
        selectPreset(itemIndex);
    }
}

}

#endif
