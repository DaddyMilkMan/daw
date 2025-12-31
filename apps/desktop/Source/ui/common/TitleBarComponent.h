/*
  ==============================================================================

    TitleBarComponent.h
    Created: 2025-12-30
    Author:  Zenith DAW

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../framework/SkiaComponent.h"
#include "../ZenithSkia.h"
#include "../framework/GlassmorphicPanel.h"
#include "../design-system/ZenithDesignSystem.h"

namespace zenith {

class TitleBarComponent : public SkiaComponent {
public:
  TitleBarComponent();
  ~TitleBarComponent() override;

  void resized() override;
  
  // Custom Skia drawing
  void drawSkia(SkCanvas* canvas) override;

  // Interaction
  void mouseDown(const juce::MouseEvent& e) override;
  void mouseDrag(const juce::MouseEvent& e) override;
  void mouseUp(const juce::MouseEvent& e) override;
  void mouseDoubleClick(const juce::MouseEvent& e) override;
  void mouseMove(const juce::MouseEvent& e) override;
  void mouseExit(const juce::MouseEvent& e) override;

  void setTransparentBackground(bool shouldBeTransparent) { transparentBackground_ = shouldBeTransparent; }
  void setShowTitle(bool shouldShow) { showTitle_ = shouldShow; repaint(); }

  std::function<void()> onClose;
  std::function<void()> onMinimize;
  std::function<void()> onMaximize;

private:
  SkFont titleFont_;
  SkPaint textPaint_;
  
  // Window buttons layout removed
  
  // Dragger
  juce::ComponentDragger dragger_;

  bool transparentBackground_ = false;
  bool showTitle_ = true;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TitleBarComponent)
};

} // namespace zenith
