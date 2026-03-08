/*
  ==============================================================================

    UnsavedChangesModal.h
    Created: 2025-12-30
    Author:  Zenith DAW

    Premium matte-black replacement for the native unsaved changes dialog.
    Ensures reliable shutdown handling and visual consistency.

  ==============================================================================
*/

#pragma once

#include "../controls/SkiaButton.h"
#include "../framework/SkiaComponent.h"
#include <functional>

namespace zenith {

class UnsavedChangesModal : public SkiaComponent {
public:
  UnsavedChangesModal();
  ~UnsavedChangesModal() override;

  void drawSkia(SkCanvas* canvas) override;
  void resized() override;

  // Callbacks
  std::function<void()> onSaveAndQuit;
  std::function<void()> onDiscardAndQuit;
  std::function<void()> onCancel;

private:
  std::unique_ptr<SkiaButton> btnSave_;
  std::unique_ptr<SkiaButton> btnDiscard_;
  std::unique_ptr<SkiaButton> btnCancel_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnsavedChangesModal)
};

} // namespace zenith
