/*
  ==============================================================================

    BrowserPanel.h
    Created: 2025-11-28
    Author:  Cassandra "Crash" O'Malley

    Preset and instrument browser with search and filtering.
    Designed to handle edge cases and large preset libraries.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SkiaComponent.h"

#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkFont.h>
#include <include/core/SkColor.h>
#include <include/core/SkColorSpace.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

class BrowserPanel : public SkiaComponent {
public:
    BrowserPanel();
    ~BrowserPanel() override = default;

    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;

    // Content management
    void setPresets(const juce::StringArray& presets);
    void setFilter(const juce::String& filter);
    
    // Callbacks
    std::function<void(const juce::String&)> onPresetSelected;

private:
    juce::StringArray allPresets_;
    juce::StringArray filteredPresets_;
    juce::String currentFilter_;
    int selectedIndex_ = -1;
    int hoverIndex_ = -1;
    int scrollOffset_ = 0;
    
    static constexpr int itemHeight_ = 30;
    static constexpr int maxVisibleItems_ = 20;

    void updateFilteredList();
    juce::Rectangle<int> getItemBounds(int index) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserPanel)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
