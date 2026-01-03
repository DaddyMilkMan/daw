/*
  ==============================================================================

    BrowserHoverPreview.h
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include "../../browser/BrowserData.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include <include/core/SkCanvas.h>
#endif

namespace zenith {

class BrowserHoverPreview : public SkiaComponent {
public:
  BrowserHoverPreview();
  ~BrowserHoverPreview() override;

  void showForItem(std::shared_ptr<BrowserItem> item, juce::Point<int> screenPos);
  void hide();

  void setWaveformData(const std::vector<float> &peaks);

  void drawSkia(SkCanvas *canvas) override;

private:
  std::shared_ptr<BrowserItem> currentItem_;
  std::vector<float> waveformPeaks_;
  
  static constexpr int previewWidth_ = 280;
  static constexpr int previewHeight_ = 120;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserHoverPreview)
};

} // namespace zenith
