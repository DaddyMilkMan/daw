/*
  ==============================================================================

    GridResolutionDropdown.h
    Created: 2025-12-26
    Author:  Zenith DAW

    A premium glassmorphic dropdown for selecting grid resolution.

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../framework/SkiaComponent.h"
#include "../controls/SkiaPopupMenu.h"
#include "ArrangerTypes.h"

namespace zenith {

class GridResolutionDropdown : public SkiaComponent {
public:
    GridResolutionDropdown();
    ~GridResolutionDropdown() override;

    void setResolution(GridResolution res);
    GridResolution getResolution() const { return currentResolution; }

    std::function<void(GridResolution)> onResolutionChanged;

    void drawSkia(SkCanvas* canvas) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

private:
    GridResolution currentResolution = GridResolution::Beat_1;
    bool isHovered = false;
    std::unique_ptr<SkiaPopupMenu> menu;

    juce::String getResolutionText(GridResolution res) const;
    void showMenu();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GridResolutionDropdown)
};

} // namespace zenith
