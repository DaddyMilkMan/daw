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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    ExportDialog.h
    Created: 2025-12-03
    Author:  Zenith DAW

    Beautiful Skia-based Export Dialog.
    "Neon Noir" style with advanced export options.


  ==============================================================================
*/

#pragma once

#include "../../commands/CommandAPI.h"
#include "../controls/SkiaButton.h"
#include "../controls/ZenithUIComponents.h"
#include "../framework/SkiaComponent.h"
#include <atomic>

namespace zenith {

class ExportProgressBar;

class ExportDialog : public SkiaComponent {
public:
  ExportDialog(CommandAPI &commandAPI);
  ~ExportDialog() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

private:
  CommandAPI &commandAPI_;

  // Format Selection
  std::unique_ptr<SkiaButton> btnWav_;
  std::unique_ptr<SkiaButton> btnFlac_;
  std::unique_ptr<SkiaButton> btnOgg_;
  std::unique_ptr<SkiaButton> btnAiff_;
  juce::String selectedFormat_ = "wav";

  // Bit Depth Selection
  std::unique_ptr<SkiaButton> btn8Bit_;
  std::unique_ptr<SkiaButton> btn16Bit_;
  std::unique_ptr<SkiaButton> btn24Bit_;
  std::unique_ptr<SkiaButton> btn32Bit_;
  int selectedBitDepth_ = 24;

  // Options
  std::unique_ptr<SkiaButton> toggleDither_;
  std::unique_ptr<SkiaButton> toggleNormalize_;
  std::unique_ptr<SkiaButton> toggleAIEnhance_;
  std::unique_ptr<SkiaButton> toggleStemExport_;
  
  // Normalization level (dB)
  float normalizeLevelDb_ = -0.1f;

  // Actions
  std::unique_ptr<SkiaButton> btnExport_;
  std::unique_ptr<SkiaButton> btnCancel_;

  // Progress tracking
  std::atomic<float> exportProgress_{0.0f};
  juce::String exportStatus_ = "";
  bool isExporting_ = false;

  std::unique_ptr<ExportProgressBar> progressBar_;

  void updateButtonStates();
  void triggerExport();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportDialog)
};

} // namespace zenith
