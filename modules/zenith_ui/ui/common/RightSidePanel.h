/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

#ifdef ZENITH_USE_SKIA

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

#endif // ZENITH_USE_SKIA

} // namespace zenith
