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
#include <juce_core/juce_core.h>

namespace zenith {

class SkiaTextEditor : public SkiaComponent {
public:
  SkiaTextEditor(const juce::String &componentName = {});
  ~SkiaTextEditor() override;

  // Text content
  void setText(const juce::String &text);
  juce::String getText() const;
  void clear();

  // Multi-line support
  void setMultiLine(bool multiLine);
  bool isMultiLine() const { return multiLine_; }

  // Read-only mode
  void setReadOnly(bool readOnly);
  bool isReadOnly() const { return readOnly_; }

  // Placeholder text
  void setTextToShowWhenEmpty(const juce::String &text, SkColor colour);

  // Font and appearance
  void setFont(const SkFont &font);
  void setTextColour(SkColor colour);
  void setBackgroundColour(SkColor colour);
  
  // Pill styling for premium look
  void setPillStyle(bool enabled);
  void setPillCornerRadius(float radius);
  bool isPillStyle() const { return pillStyle_; }

  // Scrollbars
  void setScrollbarsShown(bool show);

  // Event callbacks
  std::function<void()> onTextChange;
  std::function<void()> onReturnKey;
  std::function<void()> onEscapeKey;
  std::function<void()> onFocusLost;

  // Component interface
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  bool keyPressed(const juce::KeyPress &key, juce::Component *origin) override;
  void timerCallback() override;
  void focusLost(FocusChangeType cause) override;

private:
  juce::String text_;
  juce::String placeholderText_;
  bool multiLine_ = false;
  bool readOnly_ = false;
  bool scrollbarsShown_ = false;

  SkFont font_;
  SkColor textColour_;
  SkColor backgroundColour_;
  SkColor placeholderColour_;

  // Selection and caret
  int caretPosition_ = 0;
  int selectionStart_ = -1;
  int selectionEnd_ = -1;
  bool caretVisible_ = true;

  // Scroll offset
  float scrollX_ = 0.0f;
  float scrollY_ = 0.0f;
  
  // Pill styling
  bool pillStyle_ = false;
  float pillCornerRadius_ = 14.0f;

  // Internal methods
  void insertText(const juce::String &text);
  void deleteSelection();
  void updateCaretPosition(const juce::MouseEvent &e);
  juce::Rectangle<float> getTextBounds() const;
  void ensureCaretVisible();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaTextEditor)
};

} // namespace zenith