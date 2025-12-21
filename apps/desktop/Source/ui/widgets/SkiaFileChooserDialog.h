/*
  ==============================================================================

    SkiaFileChooserDialog.h
    Created: 2025-12-19
    Author:  Zenith DAW Team

    Modal dialog wrapper for SkiaFileChooser.
    Provides static methods for showing save/open dialogs with proper
    backdrop blur, animations, and keyboard handling.

  ==============================================================================
*/

#pragma once

#include "SkiaFileChooser.h"
#include "SkiaComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include <functional>
#include <memory>
#include <atomic>

namespace zenith {

/**
 * @brief Modal dialog that presents SkiaFileChooser with premium effects
 *
 * Use the static showOpenDialog/showSaveDialog methods to display file choosers
 * that overlay the entire application with backdrop blur.
 */
class SkiaFileChooserDialog : public SkiaComponent {
public:
  /**
   * @brief Callback when dialog closes
   * @param accepted True if user confirmed, false if cancelled
   * @param file The selected/entered file path
   */
  using Callback = std::function<void(bool accepted, const juce::File& file)>;

  /**
   * @brief Show an open file dialog
   * @param parent Parent component (dialog covers this component)
   * @param title Dialog title
   * @param initialDirectory Starting directory
   * @param filePatterns File patterns like "*.wav;*.mp3" or empty for all
   * @param callback Called when dialog closes
   */
  static void showOpenDialog(juce::Component* parent,
                             const juce::String& title,
                             const juce::File& initialDirectory,
                             const juce::String& filePatterns,
                             Callback callback);

  /**
   * @brief Show a save file dialog
   * @param parent Parent component
   * @param title Dialog title
   * @param initialDirectory Starting directory
   * @param defaultFilename Default filename to show
   * @param callback Called when dialog closes
   */
  static void showSaveDialog(juce::Component* parent,
                             const juce::String& title,
                             const juce::File& initialDirectory,
                             const juce::String& defaultFilename,
                             Callback callback);

  /**
   * @brief Show a directory chooser dialog
   * @param parent Parent component
   * @param title Dialog title
   * @param initialDirectory Starting directory
   * @param callback Called when dialog closes
   */
  static void showDirectoryDialog(juce::Component* parent,
                                  const juce::String& title,
                                  const juce::File& initialDirectory,
                                  Callback callback);

  ~SkiaFileChooserDialog() override;

  void drawSkia(SkCanvas* canvas) override;
  void resized() override;
  bool keyPressed(const juce::KeyPress& key) override;
  void mouseDown(const juce::MouseEvent& e) override;
  void timerCallback() override;

private:
  SkiaFileChooserDialog(juce::Component* parent,
                        const juce::String& title,
                        const juce::File& initialDirectory,
                        const juce::String& filePatterns,
                        const juce::String& defaultFilename,
                        SkiaFileChooser::Mode mode,
                        Callback callback);

  void show();
  void dismiss(bool accepted);
  void onChooserResult(SkiaFileChooser::Result result, const juce::File& file);

  juce::Component* parentComponent_ = nullptr;
  std::unique_ptr<SkiaFileChooser> fileChooser_;
  Callback callback_;
  juce::File resultFile_;

  // Animation
  AnimatedValue backdropAlpha_{0.0f};
  AnimatedValue dialogScale_{0.95f};
  bool isClosing_ = false;
  
  // Thread safety for async callbacks
  std::shared_ptr<std::atomic<bool>> isShuttingDown_ = 
      std::make_shared<std::atomic<bool>>(false);

  // Dialog layout
  static constexpr float DIALOG_WIDTH_RATIO = 0.75f;
  static constexpr float DIALOG_HEIGHT_RATIO = 0.8f;
  static constexpr float DIALOG_MAX_WIDTH = 1000.0f;
  static constexpr float DIALOG_MAX_HEIGHT = 700.0f;
  static constexpr float DIALOG_MIN_WIDTH = 600.0f;
  static constexpr float DIALOG_MIN_HEIGHT = 400.0f;

  SkRect getDialogBounds() const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaFileChooserDialog)
};

} // namespace zenith
