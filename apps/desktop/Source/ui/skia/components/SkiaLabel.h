/*
  ==============================================================================

    SkiaLabel.h
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based label component to replace juce::Label

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include <juce_core/juce_core.h>

namespace zenith {

class SkiaLabel : public SkiaComponent {
public:
  SkiaLabel(const juce::String &componentName = {},
            const juce::String &labelText = {});
  ~SkiaLabel() override;

  // Text content
  void setText(const juce::String &text,
               juce::NotificationType notification = juce::sendNotification);
  juce::String getText() const { return text_; }

  // Font and appearance
  void setFont(const SkFont &font);
  void
  setFont(float fontSize,
          SkFontStyle::Weight weight = SkFontStyle::Weight::kNormal_Weight);
  void setTextColour(SkColor colour);
  void setBackgroundColour(SkColor colour);
  void setBorderColour(SkColor colour);
  void setBorderWidth(float width);

  // Alignment
  enum class Justification {
    Left,
    Center,
    Right,
    TopLeft,
    TopCenter,
    TopRight,
    BottomLeft,
    BottomCenter,
    BottomRight
  };

  void setJustification(Justification justification);
  Justification getJustification() const { return justification_; }

  // Wrapping and truncation
  void setMinimumHorizontalScale(float scale);
  void setBorderSize(int left, int top, int right, int bottom);

  // Interactivity
  void setEditable(bool editable);
  bool isEditable() const { return editable_; }

  void setEditableOnSingleClick(bool editOnSingleClick);
  void setEditableOnDoubleClick(bool editOnDoubleClick);

  // Event callbacks
  std::function<void()> onTextChange;

  // Component interface
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;

private:
  juce::String text_;
  SkFont font_;
  SkColor textColour_;
  SkColor backgroundColour_;
  SkColor borderColour_;
  float borderWidth_ = 0.0f;

  Justification justification_ = Justification::Left;
  bool editable_ = false;
  bool editableOnSingleClick_ = false;
  bool editableOnDoubleClick_ = true;

  float minimumHorizontalScale_ = 0.0f;

  struct BorderSize {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
  } borderSize_;

  // Internal methods
  void drawText(SkCanvas *canvas, const SkRect &bounds);
  SkRect getTextBounds(const SkRect &componentBounds) const;
  void handleEditRequest();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaLabel)
};

} // namespace zenith