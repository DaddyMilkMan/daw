/*
  ==============================================================================

    BrowserRecentSidebar.h
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include "../../browser/BrowserModel.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

class BrowserRecentSidebar : public SkiaComponent {
public:
  explicit BrowserRecentSidebar(BrowserModel &model);
  ~BrowserRecentSidebar() override;

  void drawSkia(SkCanvas *canvas) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

  std::function<void(std::shared_ptr<BrowserItem>)> onItemSelected;

private:
  BrowserModel &model_;
  int hoverIndex_ = -1;
  static constexpr int itemHeight_ = 32;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserRecentSidebar)
};

} // namespace zenith
