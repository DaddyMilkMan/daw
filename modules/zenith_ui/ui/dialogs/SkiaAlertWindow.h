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