/*
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
#include "../framework/SkiaComponent.h"
#include "../widgets/SkiaButton.h"
#include "../widgets/ZenithUIComponents.h"


namespace zenith {

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

  // Actions
  std::unique_ptr<SkiaButton> btnExport_;
  std::unique_ptr<SkiaButton> btnCancel_;

  void updateButtonStates();
  void triggerExport();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportDialog)
};

} // namespace zenith
