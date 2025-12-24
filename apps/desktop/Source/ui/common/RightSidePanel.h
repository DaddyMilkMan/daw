/*
  ==============================================================================

    RightSidePanel.h
    Created: 2025-11-28
    Author:  David Chen + Isabella Moretti

    Layout container for Wingman console and scratch pads.

  ==============================================================================
*/

#pragma once

#include <memory>
#include <vector>

#include "../visualization/SpectraAnalyzerComponent.h"
#include "SkiaComponent.h"
#include "WingmanPanel.h" // Include full header to use unique_ptr
#include "ZenithSkia.h"
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

// Forward declarations
// Forward declarations
class CommandAPI;
class Engine;

#ifdef ZENITH_USE_SKIA

class RightSidePanel : public SkiaComponent {
public:
  RightSidePanel(CommandAPI &api, Engine &engine);
  ~RightSidePanel() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void timerCallback() override;

private:
  // Child components
  std::unique_ptr<WingmanPanel> wingmanPanel_;
  std::unique_ptr<SpectraAnalyzerComponent> spectraAnalyzer_;

  // Cached resources for 60FPS rendering
  ::SkPaint bgPaint_;
  ::SkPaint borderPaint_;
  ::SkPaint meterBgPaint_;
  ::SkPaint meterPeakPaint_;
  ::SkPaint meterRmsPaint_;
  ::SkPaint textPaint_;
  ::SkPaint subTextPaint_;
  ::SkFont headerFont_;
  ::SkFont bodyFont_;
  ::SkFont labelFont_;
  ::SkRect cachedBounds_;

  float animationPhase_ = 0.0f;

  void updateCachedPaints(const ::SkRect &bounds);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RightSidePanel)
};

#else // ZENITH_USE_SKIA

class RightSidePanel : public juce::Component {
public:
    RightSidePanel(CommandAPI&, Engine&) {}
    ~RightSidePanel() override = default;
    void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
