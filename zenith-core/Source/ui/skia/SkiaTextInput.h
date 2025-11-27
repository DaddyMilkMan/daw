/**
 * @file SkiaTextInput.h
 * @brief Beautiful GPU-accelerated text input field with native Skia rendering
 */

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>


#ifdef ZENITH_USE_SKIA
#include "SkiaComponent.h"
#include "SkiaTextRenderer.h"
#include "SkiaTheme.h"
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#include <include/effects/SkGradientShader.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

/**
 * @class SkiaTextInput
 * @brief GPU-accelerated text input with beautiful rendering
 */
class SkiaTextInput : public SkiaComponent, private juce::Timer {
public:
  SkiaTextInput(const juce::String &placeholder = {})
      : placeholder_(placeholder), text_(""), caretPosition_(0),
        isFocused_(false), caretVisible_(true), isReadOnly_(false) {
    setOpaque(false);
    setWantsKeyboardFocus(true);
    startTimer(500); // Caret blink
  }

  ~SkiaTextInput() override { stopTimer(); }

  //==========================================================================
  // Text API
  //==========================================================================

  void setText(const juce::String &newText) {
    if (text_ != newText) {
      text_ = newText;
      caretPosition_ = text_.length();
      repaint();
    }
  }

  juce::String getText() const { return text_; }

  void setReadOnly(bool readOnly) { isReadOnly_ = readOnly; }

  void setPlaceholder(const juce::String &placeholder) {
    placeholder_ = placeholder;
    repaint();
  }

  std::function<void(const juce::String &)> onTextChange;
  std::function<void()> onReturnKey;

  //==========================================================================
  // Component overrides
  //==========================================================================

  void focusGained(juce::Component::FocusChangeType) override {
    isFocused_ = true;
    repaint();
  }

  void focusLost(juce::Component::FocusChangeType) override {
    isFocused_ = false;
    repaint();
  }

  bool keyPressed(const juce::KeyPress &key) override {
    if (isReadOnly_)
      return false;

    if (key == juce::KeyPress::backspaceKey && caretPosition_ > 0) {
      text_ = text_.substring(0, caretPosition_ - 1) +
              text_.substring(caretPosition_);
      caretPosition_--;
      if (onTextChange)
        onTextChange(text_);
      repaint();
      return true;
    } else if (key == juce::KeyPress::deleteKey &&
               caretPosition_ < text_.length()) {
      text_ = text_.substring(0, caretPosition_) +
              text_.substring(caretPosition_ + 1);
      if (onTextChange)
        onTextChange(text_);
      repaint();
      return true;
    } else if (key == juce::KeyPress::returnKey) {
      if (onReturnKey)
        onReturnKey();
      return true;
    } else if (key == juce::KeyPress::leftKey && caretPosition_ > 0) {
      caretPosition_--;
      repaint();
      return true;
    } else if (key == juce::KeyPress::rightKey &&
               caretPosition_ < text_.length()) {
      caretPosition_++;
      repaint();
      return true;
    } else if (key.getTextCharacter() >= 32 && key.getTextCharacter() < 127) {
      text_ = text_.substring(0, caretPosition_) +
              juce::String::charToString(key.getTextCharacter()) +
              text_.substring(caretPosition_);
      caretPosition_++;
      if (onTextChange)
        onTextChange(text_);
      repaint();
      return true;
    }

    return false;
  }

  //==========================================================================
  // SkiaComponent implementation
  //==========================================================================

  void drawSkia(SkCanvas *canvas) override {
    SkRect bounds = SkRect::MakeWH(getWidth(), getHeight());
    const auto &theme = SkiaTheme::getInstance();
    const auto &colors = theme.getColors();

    // Background with border
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(isFocused_ ? colors.surfaceHover : colors.surfaceDefault);

    SkRRect rrect = SkRRect::MakeRectXY(bounds, 4.0f, 4.0f);
    canvas->drawRRect(rrect, bgPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(isFocused_ ? 2.0f : 1.0f);
    borderPaint.setColor(isFocused_ ? colors.primary : colors.border);
    canvas->drawRRect(rrect, borderPaint);

    // Text
    SkiaTextRenderer textRenderer;
    TextRenderOptions textOpts;
    textOpts.antiAlias = true;
    textOpts.effects = TextEffect::None;

    juce::String displayText = text_.isEmpty() ? placeholder_ : text_;
    textOpts.color =
        text_.isEmpty() ? colors.textSecondary : colors.textPrimary;

    float textX = bounds.x() + 8.0f;
    float textY = bounds.centerY() + 4.0f;

    textRenderer.drawText(canvas, displayText, textX, textY, TextStyle::Regular,
                          textOpts);

    // Draw caret if focused
    if (isFocused_ && caretVisible_ && !isReadOnly_) {
      SkRect caretBounds = textRenderer.measureText(
          text_.substring(0, caretPosition_), TextStyle::Regular);
      float caretX = textX + caretBounds.width();

      SkPaint caretPaint;
      caretPaint.setColor(colors.primary);
      caretPaint.setStrokeWidth(2.0f);
      canvas->drawLine(caretX, bounds.y() + 6, caretX, bounds.bottom() - 6,
                       caretPaint);
    }
  }

private:
  void timerCallback() override {
    if (isFocused_) {
      caretVisible_ = !caretVisible_;
      repaint();
    }
  }

  juce::String placeholder_;
  juce::String text_;
  int caretPosition_;
  bool isFocused_;
  bool caretVisible_;
  bool isReadOnly_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaTextInput)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
