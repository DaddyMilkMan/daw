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