/*
  ==============================================================================

    SkiaTextInput.cpp
    Created: 2026-01-10
    Author:  Zenith DAW

    Pure Skia text input - QUALITY IMPLEMENTATION.
    Full-featured with clipboard, selection, scroll, word boundaries.

  ==============================================================================
*/

#include "SkiaTextInput.h"
#include "../design-system/ZenithTypography.h"
#include <effects/SkGradientShader.h>

namespace zenith {

//==============================================================================
// Constructor / Destructor
//==============================================================================

SkiaTextInput::SkiaTextInput() {
  setWantsKeyboardFocus(true);
  setMouseClickGrabsKeyboardFocus(true);
  recalculateTextMetrics();
  startTimer(16); // ~60fps for cursor blink
}

SkiaTextInput::~SkiaTextInput() {
  stopTimer();
}

//==============================================================================
// Text Management
//==============================================================================

void SkiaTextInput::setText(const juce::String& newText) {
  text_ = newText.substring(0, maxLength_);
  cursorPos_ = text_.length();
  selectionAnchor_ = -1;
  scrollOffset_ = 0;
  recalculateTextMetrics();
  ensureCursorVisible();
  markDirty();
  if (onTextChanged) onTextChanged(text_);
}

void SkiaTextInput::clear() {
  text_.clear();
  cursorPos_ = 0;
  selectionAnchor_ = -1;
  scrollOffset_ = 0;
  recalculateTextMetrics();
  markDirty();
  if (onTextChanged) onTextChanged(text_);
}

void SkiaTextInput::selectAll() {
  if (text_.isEmpty()) return;
  selectionAnchor_ = 0;
  cursorPos_ = text_.length();
  markDirty();
}

void SkiaTextInput::recalculateTextMetrics() {
  cachedFont_ = design::typography::getSkFont(fontSize_, design::FontWeight::Regular);
  totalTextWidth_ = getTextWidth(text_);
}

//==============================================================================
// Drawing
//==============================================================================

void SkiaTextInput::drawSkia(SkCanvas* canvas) {
  if (!canvas) return;
  
  auto bounds = getLocalBounds().toFloat();
  SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  
  float radius = pillShape_ ? bounds.getHeight() * 0.5f : 8.0f;
  float padding = getPadding();
  textAreaWidth_ = bounds.getWidth() - padding * 2;
  
  //==========================================================================
  // 1. Background + Border (optional)
  //==========================================================================
  if (drawBackground_) {
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(design::withAlpha(design::colors::BG_02, 0.6f));
    canvas->drawRoundRect(rect, radius, radius, bgPaint);
    
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    
    if (isFocused_) {
      borderPaint.setStrokeWidth(1.5f);
      borderPaint.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.6f));
    } else {
      borderPaint.setStrokeWidth(0.5f);
      borderPaint.setColor(design::withAlpha(design::colors::BORDER_SUBTLE, 0.5f));
    }
    canvas->drawRoundRect(rect, radius, radius, borderPaint);
  }
  
  //==========================================================================
  // 3. Text Area (clipped for scroll)
  //==========================================================================
  canvas->save();
  SkRect clipRect = SkRect::MakeLTRB(padding, 0, bounds.getWidth() - padding, bounds.getHeight());
  canvas->clipRect(clipRect);
  
  float textY = bounds.getHeight() / 2.0f + fontSize_ * 0.35f;
  float textX = padding - scrollOffset_;
  
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  
  if (text_.isEmpty() && !isFocused_) {
    // Placeholder
    textPaint.setColor(placeholderColor_);
    canvas->drawString(placeholder_.toStdString().c_str(), textX, textY, cachedFont_, textPaint);
  } else {
    // Selection highlight
    if (hasSelection()) {
      int selStart = getSelectionStart();
      int selEnd = getSelectionEnd();
      
      float selStartX = padding + getTextWidthUpTo(selStart) - scrollOffset_;
      float selEndX = padding + getTextWidthUpTo(selEnd) - scrollOffset_;
      
      SkPaint selPaint;
      selPaint.setAntiAlias(true);
      selPaint.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.35f));
      
      SkRect selRect = SkRect::MakeLTRB(selStartX, 4, selEndX, bounds.getHeight() - 4);
      canvas->drawRoundRect(selRect, 3, 3, selPaint);
    }
    
    // Text
    textPaint.setColor(textColor_);
    canvas->drawString(text_.toStdString().c_str(), textX, textY, cachedFont_, textPaint);
    
    // Cursor
    if (isFocused_ && cursorVisible_) {
      float cursorX = padding + getCursorXPosition() - scrollOffset_;
      
      SkPaint cursorPaint;
      cursorPaint.setAntiAlias(true);
      cursorPaint.setColor(cursorColor_);
      cursorPaint.setStrokeWidth(2.0f);
      
      canvas->drawLine(cursorX, 6, cursorX, bounds.getHeight() - 6, cursorPaint);
    }
  }
  
  canvas->restore();
  
  //==========================================================================
  // 4. Scroll indicators (fade gradients at edges)
  //==========================================================================
  if (drawBackground_ && scrollOffset_ > 0) {
    // Left fade
    SkPaint fadePaint;
    SkPoint pts[2] = {{padding, 0}, {padding + 20, 0}};
    SkColor colors[2] = {design::withAlpha(design::colors::BG_02, 0.9f), SkColorSetA(design::colors::BG_02, 0)};
    fadePaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRect(SkRect::MakeLTRB(padding, 0, padding + 20, bounds.getHeight()), fadePaint);
  }
  
  if (drawBackground_ && totalTextWidth_ > textAreaWidth_ + scrollOffset_) {
    // Right fade
    SkPaint fadePaint;
    float rightEdge = bounds.getWidth() - padding;
    SkPoint pts[2] = {{rightEdge - 20, 0}, {rightEdge, 0}};
    SkColor colors[2] = {SkColorSetA(design::colors::BG_02, 0), design::withAlpha(design::colors::BG_02, 0.9f)};
    fadePaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRect(SkRect::MakeLTRB(rightEdge - 20, 0, rightEdge, bounds.getHeight()), fadePaint);
  }
}

void SkiaTextInput::resized() {
  textAreaWidth_ = getWidth() - getPadding() * 2;
  ensureCursorVisible();
  markDirty();
}

//==============================================================================
// Text Metrics
//==============================================================================

float SkiaTextInput::getTextWidth(const juce::String& str) const {
  if (str.isEmpty()) return 0.0f;
  SkRect bounds;
  cachedFont_.measureText(str.toUTF8(), str.getNumBytesAsUTF8(), SkTextEncoding::kUTF8, &bounds);
  return bounds.width();
}

float SkiaTextInput::getTextWidthUpTo(int charIndex) const {
  if (charIndex <= 0) return 0.0f;
  return getTextWidth(text_.substring(0, charIndex));
}

int SkiaTextInput::getCharIndexAt(float localX) const {
  if (text_.isEmpty()) return 0;
  
  float adjustedX = localX + scrollOffset_ - getPadding();
  if (adjustedX <= 0) return 0;
  
  // Binary search for optimal performance with long text
  int left = 0, right = text_.length();
  while (left < right) {
    int mid = (left + right + 1) / 2;
    float midWidth = getTextWidthUpTo(mid);
    if (midWidth <= adjustedX) {
      left = mid;
    } else {
      right = mid - 1;
    }
  }
  
  // Check if click is closer to next character
  if (left < text_.length()) {
    float leftWidth = getTextWidthUpTo(left);
    float rightWidth = getTextWidthUpTo(left + 1);
    if (adjustedX - leftWidth > rightWidth - adjustedX) {
      return left + 1;
    }
  }
  
  return left;
}

float SkiaTextInput::getCursorXPosition() const {
  return getTextWidthUpTo(cursorPos_);
}

void SkiaTextInput::ensureCursorVisible() {
  if (textAreaWidth_ <= 0) return;
  
  float cursorX = getCursorXPosition();
  float margin = 10.0f;
  
  // Cursor too far right
  if (cursorX - scrollOffset_ > textAreaWidth_ - margin) {
    scrollOffset_ = cursorX - textAreaWidth_ + margin;
  }
  // Cursor too far left
  else if (cursorX - scrollOffset_ < margin) {
    scrollOffset_ = std::max(0.0f, cursorX - margin);
  }
  
  // Clamp scroll
  float maxScroll = std::max(0.0f, totalTextWidth_ - textAreaWidth_ + 10);
  scrollOffset_ = juce::jlimit(0.0f, maxScroll, scrollOffset_);
}

//==============================================================================
// Selection Helpers
//==============================================================================

juce::String SkiaTextInput::getSelectedText() const {
  if (!hasSelection()) return {};
  return text_.substring(getSelectionStart(), getSelectionEnd());
}

void SkiaTextInput::deleteSelection() {
  if (!hasSelection()) return;
  
  int start = getSelectionStart();
  int end = getSelectionEnd();
  
  text_ = text_.substring(0, start) + text_.substring(end);
  cursorPos_ = start;
  selectionAnchor_ = -1;
  recalculateTextMetrics();
  ensureCursorVisible();
}

void SkiaTextInput::insertText(const juce::String& textToInsert) {
  if (hasSelection()) {
    deleteSelection();
  }
  
  if (text_.length() + textToInsert.length() <= maxLength_) {
    text_ = text_.substring(0, cursorPos_) + textToInsert + text_.substring(cursorPos_);
    cursorPos_ += textToInsert.length();
    recalculateTextMetrics();
    ensureCursorVisible();
  }
}

//==============================================================================
// Cursor Movement
//==============================================================================

void SkiaTextInput::moveCursor(int delta, bool extendSelection) {
  int newPos = juce::jlimit(0, text_.length(), cursorPos_ + delta);
  
  if (extendSelection) {
    if (selectionAnchor_ < 0) {
      selectionAnchor_ = cursorPos_;
    }
  } else {
    selectionAnchor_ = -1;
  }
  
  cursorPos_ = newPos;
  ensureCursorVisible();
  cursorVisible_ = true;
  blinkCounter_ = 0;
  markDirty();
}

void SkiaTextInput::moveCursorToWordBoundary(int direction, bool extendSelection) {
  int newPos;
  if (direction < 0) {
    newPos = findWordStart(cursorPos_);
  } else {
    newPos = findWordEnd(cursorPos_);
  }
  
  if (extendSelection) {
    if (selectionAnchor_ < 0) {
      selectionAnchor_ = cursorPos_;
    }
  } else {
    selectionAnchor_ = -1;
  }
  
  cursorPos_ = newPos;
  ensureCursorVisible();
  cursorVisible_ = true;
  blinkCounter_ = 0;
  markDirty();
}

//==============================================================================
// Word Boundary Detection
//==============================================================================

int SkiaTextInput::findWordStart(int fromPos) const {
  if (fromPos <= 0) return 0;
  
  int pos = fromPos - 1;
  
  // Skip whitespace
  while (pos > 0 && juce::CharacterFunctions::isWhitespace(text_[pos])) {
    pos--;
  }
  
  // Find start of word
  while (pos > 0 && !juce::CharacterFunctions::isWhitespace(text_[pos - 1])) {
    pos--;
  }
  
  return pos;
}

int SkiaTextInput::findWordEnd(int fromPos) const {
  int len = text_.length();
  if (fromPos >= len) return len;
  
  int pos = fromPos;
  
  // Skip whitespace
  while (pos < len && juce::CharacterFunctions::isWhitespace(text_[pos])) {
    pos++;
  }
  
  // Find end of word
  while (pos < len && !juce::CharacterFunctions::isWhitespace(text_[pos])) {
    pos++;
  }
  
  return pos;
}

//==============================================================================
// Clipboard Operations
//==============================================================================

void SkiaTextInput::copyToClipboard() {
  if (hasSelection()) {
    juce::SystemClipboard::copyTextToClipboard(getSelectedText());
  }
}

void SkiaTextInput::cutToClipboard() {
  if (hasSelection()) {
    copyToClipboard();
    deleteSelection();
    recalculateTextMetrics();
    if (onTextChanged) onTextChanged(text_);
    markDirty();
  }
}

void SkiaTextInput::pasteFromClipboard() {
  juce::String clipboardText = juce::SystemClipboard::getTextFromClipboard();
  
  // Sanitize: remove newlines for single-line input
  clipboardText = clipboardText.replaceCharacters("\r\n", "  ");
  
  if (clipboardText.isNotEmpty()) {
    insertText(clipboardText);
    if (onTextChanged) onTextChanged(text_);
    markDirty();
  }
}

//==============================================================================
// Input Handling
//==============================================================================

void SkiaTextInput::mouseDown(const juce::MouseEvent& e) {
  grabKeyboardFocus();
  
  int clickPos = getCharIndexAt((float)e.x);
  
  if (e.mods.isShiftDown() && isFocused_) {
    // Shift-click extends selection
    if (selectionAnchor_ < 0) {
      selectionAnchor_ = cursorPos_;
    }
    cursorPos_ = clickPos;
  } else {
    cursorPos_ = clickPos;
    selectionAnchor_ = clickPos;
  }
  
  cursorVisible_ = true;
  blinkCounter_ = 0;
  markDirty();
}

void SkiaTextInput::mouseUp(const juce::MouseEvent& e) {
  juce::ignoreUnused(e);
  // Clear selection if it's empty
  if (selectionAnchor_ == cursorPos_) {
    selectionAnchor_ = -1;
  }
}

void SkiaTextInput::mouseDrag(const juce::MouseEvent& e) {
  int dragPos = getCharIndexAt((float)e.x);
  cursorPos_ = dragPos;
  ensureCursorVisible();
  markDirty();
}

void SkiaTextInput::mouseDoubleClick(const juce::MouseEvent& e) {
  // Select word at click position
  int clickPos = getCharIndexAt((float)e.x);
  
  selectionAnchor_ = findWordStart(clickPos);
  cursorPos_ = findWordEnd(clickPos);
  
  cursorVisible_ = true;
  markDirty();
}

bool SkiaTextInput::keyPressed(const juce::KeyPress& key) {
  bool handled = true;
  bool textChanged = false;
  bool isShift = key.getModifiers().isShiftDown();
  bool isCtrl = key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown();
  
  if (key == juce::KeyPress::returnKey) {
    if (onReturnKey) onReturnKey();
  }
  else if (key == juce::KeyPress::escapeKey) {
    if (onEscapeKey) onEscapeKey();
    else unfocusAllComponents();
  }
  else if (key == juce::KeyPress::backspaceKey) {
    if (hasSelection()) {
      deleteSelection();
      textChanged = true;
    } else if (cursorPos_ > 0) {
      if (isCtrl) {
        // Delete word
        int wordStart = findWordStart(cursorPos_);
        text_ = text_.substring(0, wordStart) + text_.substring(cursorPos_);
        cursorPos_ = wordStart;
      } else {
        text_ = text_.substring(0, cursorPos_ - 1) + text_.substring(cursorPos_);
        cursorPos_--;
      }
      recalculateTextMetrics();
      ensureCursorVisible();
      textChanged = true;
    }
  }
  else if (key == juce::KeyPress::deleteKey) {
    if (hasSelection()) {
      deleteSelection();
      textChanged = true;
    } else if (cursorPos_ < text_.length()) {
      if (isCtrl) {
        // Delete word forward
        int wordEnd = findWordEnd(cursorPos_);
        text_ = text_.substring(0, cursorPos_) + text_.substring(wordEnd);
      } else {
        text_ = text_.substring(0, cursorPos_) + text_.substring(cursorPos_ + 1);
      }
      recalculateTextMetrics();
      textChanged = true;
    }
  }
  else if (key == juce::KeyPress::leftKey) {
    if (isCtrl) {
      moveCursorToWordBoundary(-1, isShift);
    } else {
      moveCursor(-1, isShift);
    }
  }
  else if (key == juce::KeyPress::rightKey) {
    if (isCtrl) {
      moveCursorToWordBoundary(1, isShift);
    } else {
      moveCursor(1, isShift);
    }
  }
  else if (key == juce::KeyPress::homeKey) {
    if (isShift && selectionAnchor_ < 0) {
      selectionAnchor_ = cursorPos_;
    } else if (!isShift) {
      selectionAnchor_ = -1;
    }
    cursorPos_ = 0;
    ensureCursorVisible();
  }
  else if (key == juce::KeyPress::endKey) {
    if (isShift && selectionAnchor_ < 0) {
      selectionAnchor_ = cursorPos_;
    } else if (!isShift) {
      selectionAnchor_ = -1;
    }
    cursorPos_ = text_.length();
    ensureCursorVisible();
  }
  else if (isCtrl && (key.getKeyCode() == 'A' || key.getTextCharacter() == 1)) {
    selectAll();
  }
  else if (isCtrl && (key.getKeyCode() == 'C' || key.getTextCharacter() == 3)) {
    copyToClipboard();
  }
  else if (isCtrl && (key.getKeyCode() == 'X' || key.getTextCharacter() == 24)) {
    cutToClipboard();
    textChanged = true;
  }
  else if (isCtrl && (key.getKeyCode() == 'V' || key.getTextCharacter() == 22)) {
    pasteFromClipboard();
    textChanged = true;
  }
  else if (key.getTextCharacter() >= 32 && !isCtrl) {
    // Printable character
    juce::String charToInsert = juce::String::charToString(key.getTextCharacter());
    insertText(charToInsert);
    textChanged = true;
  }
  else {
    handled = false;
  }
  
  if (textChanged && onTextChanged) {
    onTextChanged(text_);
  }
  
  cursorVisible_ = true;
  blinkCounter_ = 0;
  markDirty();
  
  return handled;
}

void SkiaTextInput::focusGained(FocusChangeType cause) {
  juce::ignoreUnused(cause);
  isFocused_ = true;
  cursorVisible_ = true;
  blinkCounter_ = 0;
  markDirty();
}

void SkiaTextInput::focusLost(FocusChangeType cause) {
  juce::ignoreUnused(cause);
  isFocused_ = false;
  cursorVisible_ = false;
  selectionAnchor_ = -1;
  markDirty();
  
  if (onFocusLostCallback)
      onFocusLostCallback();
}

void SkiaTextInput::timerCallback() {
  if (isFocused_) {
    blinkCounter_++;
    if (blinkCounter_ >= BLINK_RATE) {
      blinkCounter_ = 0;
      cursorVisible_ = !cursorVisible_;
      markDirty();
    }
  }
}

} // namespace zenith
