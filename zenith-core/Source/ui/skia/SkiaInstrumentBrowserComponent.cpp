/**
 * @file SkiaInstrumentBrowserComponent.cpp
 */

#include "SkiaInstrumentBrowserComponent.h"

#ifdef ZENITH_USE_SKIA

namespace zenith {

SkiaInstrumentBrowserComponent::SkiaInstrumentBrowserComponent()
{
    setSize(300, 400);
}

void SkiaInstrumentBrowserComponent::addInstrument(const juce::String& name)
{
    instruments_.push_back(name);
    repaint();
}

void SkiaInstrumentBrowserComponent::selectInstrument(int index)
{
    if (index >= 0 && index < static_cast<int>(instruments_.size()))
    {
        selectedIndex_ = index;
        repaint();
    }
}

void SkiaInstrumentBrowserComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    g.setColour(juce::Colours::white);
    g.setFont(14);
    g.drawText("Instruments", 10, 10, 200, 25, juce::Justification::left);

    int y = 40;
    for (int i = 0; i < static_cast<int>(instruments_.size()); i++)
    {
        if (i == selectedIndex_)
            g.setColour(juce::Colours::cyan);
        else
            g.setColour(juce::Colours::darkgrey);

        g.fillRect(10, y, getWidth() - 20, 30);
        g.setColour(juce::Colours::white);
        g.drawRect(10, y, getWidth() - 20, 30);
        g.drawText(instruments_[i], 15, y + 5, 200, 20, juce::Justification::left);
        y += 35;
    }
}

void SkiaInstrumentBrowserComponent::resized()
{
}

void SkiaInstrumentBrowserComponent::mouseDown(const juce::MouseEvent& event)
{
    int itemIndex = (event.y - 40) / 35;
    if (itemIndex >= 0 && itemIndex < static_cast<int>(instruments_.size()))
    {
        selectInstrument(itemIndex);
    }
}

}

#endif
