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

#include "SkiaComponent.h"
#include "browser/BrowserData.h"
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
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
