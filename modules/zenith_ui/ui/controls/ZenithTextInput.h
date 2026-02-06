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
#include "../validation/Validator.h"
#include "../validation/ValidationError.h"
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#endif

namespace zenith {

class ZenithTextInput : public SkiaComponent,
                        private juce::TextEditor::Listener,
                        public ValidatableComponent {
public:
  // ----- Input Types -----
  enum class InputType {
    Text,      // Free text
    Integer,   // Whole numbers only
    Decimal,   // Floating point
    Frequency, // Hz with suffix
    Time       // ms/s with suffix
  };

  // ----- Constructors -----
  ZenithTextInput();
  explicit ZenithTextInput(const juce::String &label);
  ~ZenithTextInput() override;

  // ----- Value -----
  void setText(const juce::String &text, bool sendNotification = true);
  juce::String getText() const { return text_; }
  void setValue(double value, bool sendNotification = true);
  double getValue() const;

  // ----- Validation -----
  void setInputType(InputType type) { inputType_ = type; }
  void setRange(double min, double max) {
    minValue_ = min;
    maxValue_ = max;
  }
  void setSuffix(const juce::String &suffix) {
    suffix_ = suffix;
    repaint();
  }
  void setPrefix(const juce::String &prefix) {
    prefix_ = prefix;
    repaint();
  }

  // ----- Validation Interface Implementation -----
  ValidationResult validate() override;
  Validator* getValidator() override { return validator_.get(); }
  void setValidator(Validator* validator) override;
  void setAutoValidate(bool autoValidate) override;
  bool getAutoValidate() const override { return autoValidate_; }
  ValidationResult getLastValidationResult() const override { return lastValidationResult_; }
  void clearValidation() override;
  void setValidationCallback(std::function<void(const ValidationResult&)> callback) override;

  // ----- Validation Helpers -----
  void validateText();
  void addError(const juce::String& error);
  void addWarning(const juce::String& warning);
  void setValidationState(const ValidationResult& result);

  // ----- Appearance -----
  void setLabel(const juce::String &label) {
    label_ = label;
    repaint();
  }
  juce::String getLabel() const { return label_; }
  void setAccentColor(SkColor color) {
    accentColor_ = color;
    repaint();
  }

  // ----- Callbacks -----
  std::function<void(const juce::String &)> onTextChanged;
  std::function<void(double)> onValueChanged;

  // ----- Rendering -----
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

protected:
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseWheelMove(const juce::MouseEvent &e,
                      const juce::MouseWheelDetails &wheel) override;
  bool keyPressed(const juce::KeyPress &key) override;

private:
  void textEditorTextChanged(juce::TextEditor &editor) override;
  void textEditorReturnKeyPressed(juce::TextEditor &editor) override;
  void textEditorEscapeKeyPressed(juce::TextEditor &editor) override;
  void textEditorFocusLost(juce::TextEditor &editor) override;

  void startEditing();
  void finishEditing(bool cancelled = false);
  void validateAndApply(const juce::String &newText);
  void incrementValue(double delta);
  juce::String formatValue(double value) const;

#ifdef ZENITH_USE_SKIA
  void drawBackground(SkCanvas *canvas);
  void drawText(SkCanvas *canvas);
  void drawLabel(SkCanvas *canvas);
#endif

  juce::String text_;
  double value_ = 0.0;
  juce::String label_;
  juce::String suffix_;
  juce::String prefix_;

  InputType inputType_ = InputType::Decimal;
  double minValue_ = -std::numeric_limits<double>::max();
  double maxValue_ = std::numeric_limits<double>::max();
  double stepSize_ = 1.0;

  bool hovered_ = false;
  bool isEditing_ = false;

  // Drag state
  juce::Point<float> dragStartPos_;
  double dragStartValue_ = 0.0;
  bool isDragging_ = false;

  std::unique_ptr<juce::TextEditor> editor_;

  SkColor accentColor_ = SkColorSetRGB(0, 255, 255);

  // Validation state
  std::unique_ptr<Validator> validator_;
  ValidationResult lastValidationResult_;
  bool autoValidate_ = true;
  std::function<void(const ValidationResult&)> validationCallback_;
  bool showValidationErrors_ = true;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithTextInput)
};

} // namespace zenith
