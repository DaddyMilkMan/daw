/*
  ==============================================================================

    BrowserFilterBar.h
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include "../../browser/BrowserModel.h"

#include <include/core/SkCanvas.h>


namespace zenith {

class BrowserFilterBar : public SkiaComponent {
public:
  explicit BrowserFilterBar(BrowserModel &model);
  ~BrowserFilterBar() override;

  void drawSkia(SkCanvas *canvas) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void resized() override;

  std::function<void()> onFilterChanged;

private:
  void drawFilterTab(SkCanvas *canvas, const juce::Rectangle<int> &bounds,
                     const juce::String &label, bool active);

  BrowserModel &model_;
  
  juce::Rectangle<int> filterAllBounds_;
  juce::Rectangle<int> filterAudioBounds_;
  juce::Rectangle<int> filterMidiBounds_;
  juce::Rectangle<int> filterPluginBounds_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserFilterBar)
};

} // namespace zenith
