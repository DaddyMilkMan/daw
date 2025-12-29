/*
  ==============================================================================

    ZenithTextInput.h
    Created: 2025-12-12
    Author:  Zenith DAW

    Premium text input for numeric entry:
    - Click to edit
    - Input range validation
    - Up/down arrow increment
    - Optional label

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#endif

namespace zenith {

class ZenithTextInput : public SkiaComponent,
                        private juce::TextEditor::Listener {
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

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithTextInput)
};

} // namespace zenith
