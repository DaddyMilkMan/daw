/*
  ==============================================================================

    ZenithTextInput.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    Implementation of the premium text input.

  ==============================================================================
*/

#include "ZenithTextInput.h"
#include "ZenithTextInput.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithDesignSystem.h"
// #include "../design-system/ZenithTheme.h" // Deprecated

#ifdef ZENITH_USE_SKIA
#include <core/SkBlurTypes.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>
#endif

namespace zenith {

ZenithTextInput::ZenithTextInput() : label_("") { setWantsKeyboardFocus(true); }

ZenithTextInput::ZenithTextInput(const juce::String &label) : label_(label) {
  setWantsKeyboardFocus(true);
}

ZenithTextInput::~ZenithTextInput() {
  if (editor_) {
    editor_->removeListener(this);
  }
}

void ZenithTextInput::setText(const juce::String &text, bool sendNotification) {
  if (text_ != text) {
    text_ = text;
    repaint();

    if (sendNotification && onTextChanged) {
      onTextChanged(text_);
    }
  }
}

void ZenithTextInput::setValue(double value, bool sendNotification) {
  value = juce::jlimit(minValue_, maxValue_, value);
  value_ = value;
  setText(formatValue(value), false);

  if (sendNotification && onValueChanged) {
    onValueChanged(value);
  }
}

double ZenithTextInput::getValue() const {
  if (inputType_ != InputType::Text)
    return value_;
  return text_.getDoubleValue();
}

void ZenithTextInput::resized() {
  if (editor_) {
    editor_->setBounds(getLocalBounds().reduced(4, 2));
  }
}

void ZenithTextInput::mouseDown(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  
  dragStartPos_ = e.position;
  dragStartValue_ = getValue();
  isDragging_ = false;
  
  // Don't start editing immediately, wait to see if it's a drag
}

void ZenithTextInput::mouseUp(const juce::MouseEvent &e) {
  if (!isDragging_ && !isEditing_ && contains(e.position.toInt())) {
     startEditing();
  }
  isDragging_ = false;
}

void ZenithTextInput::mouseDrag(const juce::MouseEvent &e) {
  if (inputType_ == InputType::Text) return; // No drag for text fields
  
  if (!isDragging_) {
     if (e.position.getDistanceFrom(dragStartPos_) > 5.0f) {
        isDragging_ = true;
     } else {
        return;
     }
  }
  
  float deltaY = dragStartPos_.y - e.position.y;
  float sensitivity = stepSize_ * 0.5f; // Pixels to value
  if (e.mods.isShiftDown()) sensitivity *= 0.1f;
  
  double newValue = dragStartValue_ + deltaY * sensitivity;
  setValue(newValue, true);
}

void ZenithTextInput::mouseEnter(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  hovered_ = true;
  repaint();
}

void ZenithTextInput::mouseMove(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  if (!isEditing_) {
     setMouseCursor(juce::MouseCursor::IBeamCursor);
  }
}

void ZenithTextInput::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  hovered_ = false;
  repaint();
}

void ZenithTextInput::mouseWheelMove(const juce::MouseEvent &e,
                                     const juce::MouseWheelDetails &wheel) {
  if (inputType_ != InputType::Text) {
    double delta = wheel.deltaY * stepSize_;
    if (e.mods.isShiftDown()) {
      delta *= 0.1;
    }
    incrementValue(delta);
  } else {
    Component::mouseWheelMove(e, wheel);
  }
}

bool ZenithTextInput::keyPressed(const juce::KeyPress &key) {
  if (inputType_ != InputType::Text) {
    if (key == juce::KeyPress::upKey) {
      incrementValue(key.getModifiers().isShiftDown() ? stepSize_ * 0.1
                                                      : stepSize_);
      return true;
    }
    if (key == juce::KeyPress::downKey) {
      incrementValue(key.getModifiers().isShiftDown() ? -stepSize_ * 0.1
                                                      : -stepSize_);
      return true;
    }
  }
  return false;
}

void ZenithTextInput::textEditorTextChanged(juce::TextEditor &editor) {
  juce::ignoreUnused(editor);
  // Live validation could go here
}

void ZenithTextInput::textEditorReturnKeyPressed(juce::TextEditor &editor) {
  juce::ignoreUnused(editor);
  finishEditing();
}

void ZenithTextInput::textEditorEscapeKeyPressed(juce::TextEditor &editor) {
  juce::ignoreUnused(editor);
  finishEditing(true);
}

void ZenithTextInput::textEditorFocusLost(juce::TextEditor &editor) {
  juce::ignoreUnused(editor);
  finishEditing();
}

void ZenithTextInput::startEditing() {
  isEditing_ = true;

  editor_ = std::make_unique<juce::TextEditor>();
  editor_->setMultiLine(false);
  editor_->setReturnKeyStartsNewLine(false);
  editor_->setText(text_, false);
  editor_->selectAll();
  editor_->addListener(this);

  // Style the editor
  editor_->setColour(juce::TextEditor::backgroundColourId,
                     juce::Colours::transparentBlack);
  editor_->setColour(juce::TextEditor::textColourId, design::toJuceColour(design::colors::TEXT_PRIMARY));
  editor_->setColour(juce::TextEditor::highlightColourId,
                     design::toJuceColour(design::colors::ACCENT_PRIMARY).withAlpha(0.3f));
  editor_->setColour(juce::TextEditor::outlineColourId,
                     juce::Colours::transparentBlack);
  editor_->setColour(juce::TextEditor::focusedOutlineColourId,
                     juce::Colours::transparentBlack);

  addAndMakeVisible(editor_.get());
  editor_->setBounds(getLocalBounds().reduced(4, 2));
  editor_->grabKeyboardFocus();

  repaint();
}

void ZenithTextInput::finishEditing(bool cancelled) {
  if (!isEditing_)
    return;

  if (!cancelled && editor_) {
    validateAndApply(editor_->getText());
  }

  if (editor_) {
    removeChildComponent(editor_.get());
    editor_.reset();
  }

  isEditing_ = false;
  repaint();
}

void ZenithTextInput::validateAndApply(const juce::String &newText) {
  juce::String validatedText = newText;

  if (inputType_ != InputType::Text) {
    double value = newText.getDoubleValue();
    setValue(value, true);
  } else {
    setText(validatedText, true);
  }
}

void ZenithTextInput::incrementValue(double delta) {
  double currentValue = getValue();
  double newValue = juce::jlimit(minValue_, maxValue_, currentValue + delta);
  setValue(newValue, true);
}

juce::String ZenithTextInput::formatValue(double value) const {
  juce::String result;

  switch (inputType_) {
  case InputType::Integer:
    result = juce::String(static_cast<int>(value));
    break;
  case InputType::Decimal:
    result = juce::String(value, 2);
    break;
  case InputType::Frequency:
    if (value >= 1000)
      result = juce::String(value / 1000.0, 2) + " kHz";
    else
      result = juce::String(value, 1) + " Hz";
    break;
  case InputType::Time:
    if (value >= 1000)
      result = juce::String(value / 1000.0, 2) + " s";
    else
      result = juce::String(value, 1) + " ms";
    break;
  default:
    result = juce::String(value);
    break;
  }

  return result;
}

void ZenithTextInput::drawSkia(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  if (canvas == nullptr)
    return;

  drawBackground(canvas);

  if (!isEditing_) {
    drawText(canvas);
  }

  if (label_.isNotEmpty()) {
    drawLabel(canvas);
  }
#else
  juce::ignoreUnused(canvas);
#endif
}

#ifdef ZENITH_USE_SKIA

void ZenithTextInput::drawBackground(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  SkRRect rrect = SkRRect::MakeRectXY(rect, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM);

  SkPaint paint;
  paint.setAntiAlias(true);

  // Background
  paint.setStyle(SkPaint::kFill_Style);
  SkColor bg = design::colors::BG_03;
  paint.setColor(design::withAlpha(bg, isEditing_ ? 0.7f : (hovered_ ? 0.55f : 0.47f))); // Approximate alphas
  canvas->drawRRect(rrect, paint);

  // Border/focus ring
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);

  if (isEditing_) {
    SkColor accent = design::colors::ACCENT_PRIMARY;
    paint.setColor(accent);
    paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 3.0f));
    canvas->drawRRect(rrect, paint);
    paint.setMaskFilter(nullptr);
  }

  SkColor border = design::colors::BORDER_DEFAULT;
  paint.setColor(design::withAlpha(border, hovered_ ? 0.39f : 0.23f)); // 100/255 -> 0.39
  canvas->drawRRect(rrect, paint);
}

void ZenithTextInput::drawText(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  SkFont font;
  font.setSize(12.0f);
  font.setSubpixel(true);

  SkPaint paint;
  paint.setAntiAlias(true);
  SkColor txtMain = design::colors::TEXT_PRIMARY;
  SkColor txtPlace = design::colors::TEXT_TERTIARY;
  
  if (text_.isEmpty()) {
      paint.setColor(design::withAlpha(txtPlace, 0.39f)); // 100/255
  } else {
      paint.setColor(design::withAlpha(txtMain, 0.86f)); // 220/255
  }

  juce::String displayText = text_.isEmpty() ? "0" : text_;
  if (prefix_.isNotEmpty()) {
    displayText = prefix_ + displayText;
  }
  if (suffix_.isNotEmpty()) {
    displayText = displayText + suffix_;
  }

  std::string str = displayText.toStdString();
  float textY = bounds.getHeight() / 2 + 4.0f;

  canvas->drawSimpleText(str.c_str(), str.length(), SkTextEncoding::kUTF8, 8.0f,
                         textY, font, paint);
}

void ZenithTextInput::drawLabel(SkCanvas *canvas) {
  // Label could be drawn above the input if needed
  juce::ignoreUnused(canvas);
}

#endif // ZENITH_USE_SKIA



} // namespace zenith
