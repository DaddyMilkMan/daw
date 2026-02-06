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
#include "browser/BrowserPreviewEngine.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#endif

namespace zenith {

class BrowserPreviewPanel : public SkiaComponent {
public:
  explicit BrowserPreviewPanel(BrowserPreviewEngine &engine);
  ~BrowserPreviewPanel() override;

  void drawSkia(SkCanvas *canvas) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void resized() override;

  void loadWaveform(const juce::File &file);
  void clearWaveform();

private:
  void drawWaveform(SkCanvas *canvas, const SkRect &bounds);
  void drawIconButton(SkCanvas *canvas, const juce::Rectangle<int> &bounds, const SkPath &iconPath, bool active);
  void drawButton(SkCanvas *canvas, const juce::Rectangle<int> &bounds, const juce::String &text, bool active);

  BrowserPreviewEngine &engine_;
  
  std::vector<float> waveformData_;
  juce::File waveformFile_;

  juce::Rectangle<int> playButtonBounds_;
  juce::Rectangle<int> stopButtonBounds_;
  juce::Rectangle<int> loopButtonBounds_;
  juce::Rectangle<int> autoPlayButtonBounds_;
  juce::Rectangle<int> waveformBounds_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserPreviewPanel)
};

} // namespace zenith
