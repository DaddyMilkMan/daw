/*
  ==============================================================================

    SkiaAlertWindow.h
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based alert window component to replace juce::AlertWindow

  ==============================================================================
*/

#pragma once

#include "../controls/SkiaButton.h"
#include "../framework/SkiaComponent.h"
#include "../controls/SkiaLabel.h"
#include "../controls/SkiaTextEditor.h"
#include <juce_core/juce_core.h>


namespace zenith {

class SkiaAlertWindow : public SkiaComponent {
public:
  enum class IconType { NoIcon, QuestionIcon, WarningIcon, InfoIcon };

  enum class Result { Cancelled, Button1, Button2, Button3 };

  using Callback = std::function<void(Result result)>;

  SkiaAlertWindow(const juce::String &title, const juce::String &message,
                  IconType iconType = IconType::NoIcon);
  ~SkiaAlertWindow() override;

  // Button configuration
  void addButton(const juce::String &text, Result result,
                 SkiaButton::Style style = SkiaButton::Style::Primary);

  // Text input
  void addTextEditor(const juce::String &name, const juce::String &initialText,
                     const juce::String &labelText, bool isPassword = false);
  juce::String getTextEditorContents(const juce::String &name) const;

  // Show modal
  void showAsync(Callback callback);

  // Static convenience methods
  static void showMessageBoxAsync(IconType iconType, const juce::String &title,
                                  const juce::String &message,
                                  const juce::String &buttonText = "OK");

  static void showAsync(IconType iconType, const juce::String &title,
                        const juce::String &message,
                        const juce::String &button1Text,
                        const juce::String &button2Text = {},
                        const juce::String &button3Text = {},
                        std::function<void(int)> callback = {});

  // Test Mode: Auto-dismiss windows to prevent leaks in headless tests
  static void setTestMode(bool enabled) { testModeEnabled_ = enabled; }

  // Component interface
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

private:
  struct ButtonInfo {
    juce::String text;
    Result result;
    SkiaButton::Style style;
  };

  struct TextEditorInfo {
    juce::String name;
    std::unique_ptr<SkiaTextEditor> editor;
    std::unique_ptr<SkiaLabel> label;
    bool isPassword;
  };

  // UI Components
  std::unique_ptr<SkiaLabel> titleLabel_;
  std::unique_ptr<SkiaLabel> messageLabel_;
  std::unique_ptr<SkiaLabel> iconLabel_;
  juce::Array<ButtonInfo> buttons_;
  juce::Array<TextEditorInfo> textEditors_;
  juce::OwnedArray<SkiaButton> buttonComponents_;

  // State
  IconType iconType_;
  Callback callback_;
  bool isShowing_ = false;

  // Appearance
  SkColor backgroundColour_;
  SkColor borderColour_;
  SkFont font_;

  // Layout
  int buttonHeight_ = 32;
  int buttonSpacing_ = 10;
  int margin_ = 20;
  int iconSize_ = 40;

  // Internal methods
  void layoutComponents();
  void handleButtonPressed(Result result);
  void hideWindow();
  
  static bool testModeEnabled_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaAlertWindow)
};

} // namespace zenith