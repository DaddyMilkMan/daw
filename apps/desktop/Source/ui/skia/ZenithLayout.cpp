/*
  ==============================================================================

    ZenithLayout.cpp
    Created: 2025-12-03
    Author:  Zenith DAW

    Implementation of ZenithLayout wrapper.

  ==============================================================================
*/

#include "ZenithLayout.h"

namespace zenith {

void ZenithLayout::row(const juce::Rectangle<int>& bounds, 
                      const std::vector<juce::Component*>& items, 
                      float gap,
                      juce::FlexBox::JustifyContent justify,
                      juce::FlexBox::AlignItems align)
{
    juce::FlexBox flex;
    flex.flexDirection = juce::FlexBox::Direction::row;
    flex.justifyContent = justify;
    flex.alignItems = align;
    
    for (auto* item : items) {
        if (item) {
            flex.items.add(juce::FlexItem(*item).withFlex(1.0f).withMargin(gap / 2.0f));
        }
    }
    
    flex.performLayout(bounds);
}

void ZenithLayout::column(const juce::Rectangle<int>& bounds, 
                         const std::vector<juce::Component*>& items, 
                         float gap,
                         juce::FlexBox::JustifyContent justify,
                         juce::FlexBox::AlignItems align)
{
    juce::FlexBox flex;
    flex.flexDirection = juce::FlexBox::Direction::column;
    flex.justifyContent = justify;
    flex.alignItems = align;
    
    for (auto* item : items) {
        if (item) {
            flex.items.add(juce::FlexItem(*item).withFlex(1.0f).withMargin(gap / 2.0f));
        }
    }
    
    flex.performLayout(bounds);
}

void ZenithLayout::grid(const juce::Rectangle<int>& bounds, 
                       const std::vector<juce::Component*>& items, 
                       int cols, 
                       float gapX, 
                       float gapY)
{
    if (cols <= 0) return;
    
    juce::FlexBox flex;
    flex.flexDirection = juce::FlexBox::Direction::row;
    flex.flexWrap = juce::FlexBox::Wrap::wrap;
    flex.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    flex.alignContent = juce::FlexBox::AlignContent::flexStart;
    
    float itemWidth = (float)bounds.getWidth() / cols;
    // height will be determined by items or we can force square/fixed aspect ratio if needed
    // For a generic grid, let's assume items want to fill the width slot
    
    for (auto* item : items) {
        if (item) {
            // Calculate margins
            auto margin = juce::FlexItem::Margin(gapY / 2.0f, gapX / 2.0f, gapY / 2.0f, gapX / 2.0f);
            
            flex.items.add(juce::FlexItem(*item)
                .withWidth(itemWidth - gapX)
                .withHeight(itemWidth - gapX) // Force square for now, or parameterize
                .withMargin(margin));
        }
    }
    
    flex.performLayout(bounds);
}

} // namespace zenith
