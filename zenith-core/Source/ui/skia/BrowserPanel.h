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
#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkPaint.h>
#include <skia/include/core/SkFont.h>
#include <skia/include/core/SkColor.h>
#include <skia/include/core/SkColorSpace.h>
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
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    // Content management
    void setPresets(const juce::StringArray& presets);
    void setSearchText(const juce::String& text);
    
    // Callbacks
    std::function<void(const juce::String&)> onPresetSelected;
    std::function<void(const juce::String&)> onSearchChanged;

private:
    juce::StringArray allPresets_;
    juce::StringArray filteredPresets_;
    juce::String searchText_;
    int selectedIndex_ = -1;
    int hoverIndex_ = -1;
    int scrollOffset_ = 0;
    
    // Search box bounds
    juce::Rectangle<int> searchBoxBounds_;
    bool searchBoxActive_ = false;
    
    static constexpr int headerHeight_ = 40;
    static constexpr int searchBoxHeight_ = 30;
    static constexpr int itemHeight_ = 30;
    static constexpr int maxVisibleItems_ = 20;
    static constexpr int maxVisibleItems_ = 20;

    void updateFilteredList();
    juce::Rectangle<int> getItemBounds(int index) const;
    bool keyPressed(const juce::KeyPress& key) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserPanel)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
