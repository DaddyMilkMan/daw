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
#include <skia/include/core/SkMaskFilter.h>

namespace zenith {

SkiaTextEditor::SkiaTextEditor(const juce::String &componentName) {
  setName(componentName);
  setWantsKeyboardFocus(true);

  // Set default appearance
  font_ = zenith::design::getSkFont(design::typography::FONT_MD,
                                    design::FontWeight::Regular);
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

void SkiaTextEditor::setPillStyle(bool enabled) {
  if (pillStyle_ != enabled) {
    pillStyle_ = enabled;
    if (enabled) {
      // Apply pill defaults: white bg, black text
      backgroundColour_ = SK_ColorWHITE;
      textColour_ = SK_ColorBLACK;
    }
    markDirty();
  }
}

void SkiaTextEditor::setPillCornerRadius(float radius) {
  if (pillCornerRadius_ != radius) {
    pillCornerRadius_ = radius;
    markDirty();
  }
}

void SkiaTextEditor::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  
  if (pillStyle_) {
    // Premium pill style with shadow and rounded corners
    SkRRect rrect = SkRRect::MakeRectXY(rect, pillCornerRadius_, pillCornerRadius_);
    
    // Subtle drop shadow
    SkPaint shadowPaint;
    shadowPaint.setColor(design::withAlpha(SK_ColorBLACK, 0.15f));
    shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
    shadowPaint.setAntiAlias(true);
    canvas->save();
    canvas->translate(0, 2);
    canvas->drawRRect(rrect, shadowPaint);
    canvas->restore();
    
    // White background
    SkPaint bgPaint;
    bgPaint.setColor(backgroundColour_);
    bgPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, bgPaint);
    
    // Subtle border when not focused
    if (!isFocused()) {
      SkPaint borderPaint;
      borderPaint.setColor(design::withAlpha(SK_ColorBLACK, 0.1f));
      borderPaint.setStyle(SkPaint::kStroke_Style);
      borderPaint.setStrokeWidth(1.0f);
      borderPaint.setAntiAlias(true);
      canvas->drawRRect(rrect, borderPaint);
    } else {
      // Cyan glow on focus
      SkPaint focusPaint;
      focusPaint.setColor(design::withAlpha(design::colors::CYAN, 0.4f));
      focusPaint.setStyle(SkPaint::kStroke_Style);
      focusPaint.setStrokeWidth(2.0f);
      focusPaint.setAntiAlias(true);
      canvas->drawRRect(rrect, focusPaint);
    }
  } else {
    // Original rectangle style
    SkPaint bgPaint;
    bgPaint.setColor(backgroundColour_);
    canvas->drawRect(rect, bgPaint);

    SkPaint borderPaint;
    borderPaint.setColor(isFocused() ? design::colors::BORDER_FOCUS
                                     : design::colors::BORDER_DEFAULT);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas->drawRect(rect, borderPaint);
  }

  // Draw text or placeholder with proper padding
  float textPadding = pillStyle_ ? 16.0f : 8.0f;
  SkPaint textPaint;
  textPaint.setColor(text_.isEmpty() ? placeholderColour_ : textColour_);
  textPaint.setAntiAlias(true);

  const juce::String &textToDraw = text_.isEmpty() ? placeholderText_ : text_;

  if (!textToDraw.isEmpty()) {
    canvas->drawString(textToDraw.toRawUTF8(), textPadding,
                       bounds.getHeight() * 0.5f + font_.getSize() * 0.3f,
                       font_, textPaint);
  }

  // Draw caret if focused and visible
  if (isFocused() && caretVisible_ && !readOnly_) {
    float caretX = textPadding;
    if (!text_.isEmpty()) {
      caretX += font_.measureText(text_.substring(0, caretPosition_).toRawUTF8(),
                                   text_.substring(0, caretPosition_).length(),
                                   SkTextEncoding::kUTF8);
    }

    SkPaint caretPaint;
    caretPaint.setColor(textColour_);
    caretPaint.setStrokeWidth(2.0f);
    canvas->drawLine(caretX, 6.0f, caretX, bounds.getHeight() - 6.0f, caretPaint);
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

  // Handle escape key
  if (keyCode == juce::KeyPress::escapeKey) {
    if (onEscapeKey) {
      onEscapeKey();
    }
    return true;
  }

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

  // Handle left arrow
  if (keyCode == juce::KeyPress::leftKey) {
    if (caretPosition_ > 0) {
      caretPosition_--;
      caretVisible_ = true;
      markDirty();
    }
    return true;
  }

  // Handle right arrow
  if (keyCode == juce::KeyPress::rightKey) {
    if (caretPosition_ < text_.length()) {
      caretPosition_++;
      caretVisible_ = true;
      markDirty();
    }
    return true;
  }

  // Handle home key
  if (keyCode == juce::KeyPress::homeKey) {
    caretPosition_ = 0;
    caretVisible_ = true;
    markDirty();
    return true;
  }

  // Handle end key  
  if (keyCode == juce::KeyPress::endKey) {
    caretPosition_ = text_.length();
    caretVisible_ = true;
    markDirty();
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

void SkiaTextEditor::focusLost(FocusChangeType cause) {
  juce::ignoreUnused(cause);
  caretVisible_ = false;
  markDirty();
  
  if (onFocusLost) {
    onFocusLost();
  }
}

} // namespace zenith