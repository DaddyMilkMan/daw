/*
  ==============================================================================

    SkiaTextEditor.h
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based text editor component to replace juce::TextEditor

  ==============================================================================
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
  void selectAll() { selectionStart_ = 0; selectionEnd_ = (int)text_.length(); }

  // Multi-line support
  void setMultiLine(bool multiLine);
  bool isMultiLine() const { return multiLine_; }

  // Read-only mode
  void setReadOnly(bool readOnly);
  bool isReadOnly() const { return readOnly_; }

  // Placeholder text
  void setTextToShowWhenEmpty(const juce::String &text, SkColor colour);

  // Password mode
  void setPasswordMode(bool isPassword);
  bool isPasswordMode() const { return isPassword_; }

  // Font and appearance
  void setFont(const SkFont &font);
  void setTextColour(SkColor colour);
  void setBackgroundColour(SkColor colour);

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

private:
  juce::String text_;
  juce::String placeholderText_;
  bool multiLine_ = false;
  bool readOnly_ = false;
  bool scrollbarsShown_ = false;
  bool isPassword_ = false;

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

  // Internal methods
  void insertText(const juce::String &text);
  void deleteSelection();
  void updateCaretPosition(const juce::MouseEvent &e);
  juce::Rectangle<float> getTextBounds() const;
  void ensureCaretVisible();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaTextEditor)
};

} // namespace zenith