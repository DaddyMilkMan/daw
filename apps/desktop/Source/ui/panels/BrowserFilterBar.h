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

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#endif

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
  juce::Rectangle<int> filterInstrumentsBounds_;
  juce::Rectangle<int> filterSoundsBounds_;
  juce::Rectangle<int> filterEffectsBounds_;
  juce::Rectangle<int> filterMidiBounds_;
  juce::Rectangle<int> filterPresetsBounds_;
  juce::Rectangle<int> filterProjectsBounds_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserFilterBar)
};

} // namespace zenith
