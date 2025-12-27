/*
  ==============================================================================

    SkiaTextEditor.cpp
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based text editor implementation

  ==============================================================================
*/

#include "SkiaTextEditor.h"
#include "ZenithDesignSystem.h"

namespace zenith {

SkiaTextEditor::SkiaTextEditor(const juce::String &componentName) {
  setName(componentName);

  // Set default appearance
  font_.setSize(design::typography::FONT_MD);
  textColour_ = design::colors::TEXT_PRIMARY;
  backgroundColour_ = design::colors::BG_DARK;
  placeholderColour_ = design::withAlpha(design::colors::TEXT_PRIMARY, 0.5f);

  // Start caret blink timer
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(2); // 2Hz for caret blinking
}

SkiaTextEditor::~SkiaTextEditor() { stopTimer(); }

void SkiaTextEditor::setText(const juce::String &text) {
  if (text_ != text) {
    text_ = text;
    caretPosition_ = text_.length();
    markDirty();

    if (onTextChange) {
      onTextChange();
    }
  }
}

juce::String SkiaTextEditor::getText() const { return text_; }

void SkiaTextEditor::clear() { setText({}); }

void SkiaTextEditor::setMultiLine(bool multiLine) {
  if (multiLine_ != multiLine) {
    multiLine_ = multiLine;
    markDirty();
  }
}

void SkiaTextEditor::setReadOnly(bool readOnly) { readOnly_ = readOnly; }

void SkiaTextEditor::setTextToShowWhenEmpty(const juce::String &text,
                                            SkColor colour) {
  placeholderText_ = text;
  placeholderColour_ = colour;
  markDirty();
}

void SkiaTextEditor::setFont(const SkFont &font) {
  font_ = font;
  markDirty();
}

void SkiaTextEditor::setTextColour(SkColor colour) {
  textColour_ = colour;
  markDirty();
}

void SkiaTextEditor::setBackgroundColour(SkColor colour) {
  backgroundColour_ = colour;
  markDirty();
}

void SkiaTextEditor::setScrollbarsShown(bool show) {
  scrollbarsShown_ = show;
  markDirty();
}

void SkiaTextEditor::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  // Draw background
  SkPaint bgPaint;
  bgPaint.setColor(backgroundColour_);
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                   bgPaint);

  // Draw border
  SkPaint borderPaint;
  borderPaint.setColor(isFocused() ? design::colors::BORDER_FOCUS
                                   : design::colors::BORDER_DEFAULT);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setAntiAlias(true);
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                   borderPaint);

  // Draw text or placeholder
  SkPaint textPaint;
  textPaint.setColor(text_.isEmpty() ? placeholderColour_ : textColour_);
  textPaint.setAntiAlias(true);

  const juce::String &textToDraw = text_.isEmpty() ? placeholderText_ : text_;

  if (!textToDraw.isEmpty()) {
    // Simple single-line text drawing (multi-line support can be added later)
    canvas->drawString(textToDraw.toRawUTF8(), 8.0f,
                       bounds.getHeight() * 0.5f + font_.getSize() * 0.3f,
                       font_, textPaint);
  }

  // Draw caret if focused and visible
  if (isFocused() && caretVisible_ && !readOnly_ && !text_.isEmpty()) {
    // Calculate caret position (simplified)
    float caretX =
        8.0f + font_.measureText(text_.substring(0, caretPosition_).toRawUTF8(),
                                 text_.substring(0, caretPosition_).length(),
                                 SkTextEncoding::kUTF8);

    SkPaint caretPaint;
    caretPaint.setColor(textColour_);
    caretPaint.setStrokeWidth(1.0f);
    canvas->drawLine(caretX, 4.0f, caretX, bounds.getHeight() - 4.0f,
                     caretPaint);
  }
}

void SkiaTextEditor::resized() {
  // Layout logic can be added here
}

void SkiaTextEditor::mouseDown(const juce::MouseEvent &e) {
  if (readOnly_)
    return;

  grabKeyboardFocus();
  updateCaretPosition(e);
}

void SkiaTextEditor::mouseDrag(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  // Selection logic can be added here
}

void SkiaTextEditor::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
}

void SkiaTextEditor::mouseDoubleClick(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  // Select word logic can be added here
}

bool SkiaTextEditor::keyPressed(const juce::KeyPress &key,
                                juce::Component *origin) {
  juce::ignoreUnused(origin);

  if (readOnly_)
    return false;

  auto keyCode = key.getKeyCode();

  // Handle return key
  if (keyCode == juce::KeyPress::returnKey) {
    if (onReturnKey) {
      onReturnKey();
    }
    return true;
  }

  // Handle backspace
  if (keyCode == juce::KeyPress::backspaceKey) {
    if (caretPosition_ > 0) {
      text_ = text_.substring(0, caretPosition_ - 1) +
              text_.substring(caretPosition_);
      caretPosition_--;
      markDirty();

      if (onTextChange) {
        onTextChange();
      }
    }
    return true;
  }

  // Handle character input
  auto textChar = key.getTextCharacter();
  if (textChar != 0) {
    insertText(juce::String::charToString(textChar));
    return true;
  }

  return false;
}

void SkiaTextEditor::insertText(const juce::String &text) {
  text_ = text_.substring(0, caretPosition_) + text +
          text_.substring(caretPosition_);
  caretPosition_ += text.length();
  markDirty();

  if (onTextChange) {
    onTextChange();
  }
}

void SkiaTextEditor::deleteSelection() {
  // Selection deletion logic can be added here
}

void SkiaTextEditor::updateCaretPosition(const juce::MouseEvent &e) {
  // Simplified caret position calculation
  float clickX = e.position.x;

  // Find character position based on click position
  int pos = 0;
  float currentX = 8.0f; // Left padding

  for (int i = 0; i <= text_.length(); i++) {
    float charWidth = 0;
    if (i < text_.length()) {
      charWidth = font_.measureText(text_.substring(i, i + 1).toRawUTF8(), 1,
                                    SkTextEncoding::kUTF8);
    }

    if (clickX < currentX + charWidth * 0.5f) {
      pos = i;
      break;
    }

    currentX += charWidth;
    pos = i + 1;
  }

  caretPosition_ = juce::jlimit(0, text_.length(), pos);
}

juce::Rectangle<float> SkiaTextEditor::getTextBounds() const {
  return getLocalBounds().toFloat().reduced(8.0f, 4.0f);
}

void SkiaTextEditor::ensureCaretVisible() {
  // Scroll logic can be added here
}

void SkiaTextEditor::timerCallback() {
  // Blink caret
  if (isFocused()) {
    caretVisible_ = !caretVisible_;
    markDirty();
  }
}

} // namespace zenith