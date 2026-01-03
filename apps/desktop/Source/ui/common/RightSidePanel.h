/*
  ==============================================================================

    RightSidePanel.h
    Created: 2025-11-28
    Author:  David Chen + Isabella Moretti

    Layout container for Wingman console and scratch pads.

  ==============================================================================
*/

#pragma once

#include "WingmanPanel.h" // Include full header to use unique_ptr
#include "UndoHistoryPanel.h"
#include "../controls/SpectraAnalyzerComponent.h"
#include "SkiaComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_core/juce_core.h>
#include <core/SkCanvas.h>
#include <core/SkFont.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>


namespace zenith {

// Forward declarations
class CommandAPI;
class Engine;
class ProjectState;


class RightSidePanel : public SkiaComponent {
public:
  RightSidePanel(CommandAPI &api, Engine &engine, ProjectState &projectState);
  ~RightSidePanel() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

private:
  // Child components
  std::unique_ptr<WingmanPanel> wingmanPanel_;
  std::unique_ptr<SpectraAnalyzerComponent> spectraAnalyzer_;
  std::unique_ptr<UndoHistoryPanel> undoHistoryPanel_;

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

  void updateCachedPaints(const ::SkRect &bounds);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RightSidePanel)
};


} // namespace zenith
