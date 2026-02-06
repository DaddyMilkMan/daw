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

// ZenithControl.h


#include "SkiaComponent.h"
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

/**
 * Base class for Zenith controls with common hover/value logic.
 * Implements thread-safe parameter listening per JUCE best practices.
 */
class ZenithControl : public SkiaComponent,
                      public juce::AudioProcessorParameter::Listener {
public:
  explicit ZenithControl(const juce::String &name);
  ~ZenithControl() override;

  // ----- Parameter Binding -----
  void setParameter(juce::RangedAudioParameter *param);
  juce::RangedAudioParameter *getParameter() const { return parameter_; }

  // ----- Value Access -----
  void setValue(float value, bool sendNotification = true);
  float getValue() const;
  float getNormalizedValue() const;
  void setDefaultValue(float defaultVal) { defaultValue_ = defaultVal; }
  float getDefaultValue() const { return defaultValue_; }
  void resetToDefault();
  void updateFromParameter();

  // ----- Range -----
  void setRange(float min, float max, float interval = 0.0f);
  juce::NormalisableRange<float> getRange() const { return range_; }

  // ----- Appearance -----
  void setAccentColor(SkColor color) {
    accentColor_ = color;
    repaint();
  }
  SkColor getAccentColor() const { return accentColor_; }

  void setLabel(const juce::String &label) {
    name_ = label;
    repaint();
  }
  juce::String getLabel() const { return name_; }

  void setTextSuffix(const juce::String &suffix) { textSuffix_ = suffix; }
  juce::String getTextSuffix() const { return textSuffix_; }

  // ----- Tooltip -----
  void setTooltip(const juce::String &text) { tooltipText_ = text; }
  juce::String getTooltip() const { return tooltipText_; }

  // ----- Value Display -----
  void setShowValueOnHover(bool show) { showValueOnHover_ = show; }
  void setShowValueWhileDragging(bool show) { showValueWhileDragging_ = show; }
  juce::String getValueAsText() const;

  // ----- Interaction Modes -----
  void setFineControlMultiplier(float mult) { fineControlMultiplier_ = mult; }
  void setScrollWheelEnabled(bool enabled) { scrollWheelEnabled_ = enabled; }
  void setDoubleClickToReset(bool enabled) { doubleClickToReset_ = enabled; }

  // ----- State Queries -----
  bool isHovered() const { return isHovered_; }
  bool isDragging() const { return isDragging_; }
  bool isFineMode() const { return isFineMode_; }

  // ----- Callbacks -----
  std::function<void(ZenithControl *)> onHoverStateChanged;
  std::function<void(float)> onValueChanged;
  std::function<void()> onDragStart;
  std::function<void()> onDragEnd;

  // ----- AudioProcessorParameter::Listener -----
  void parameterValueChanged(int parameterIndex, float newValue) override;
  void parameterGestureChanged(int parameterIndex,
                               bool gestureIsStarting) override;

protected:
  // ----- Protected Members -----
  juce::String name_;
  juce::RangedAudioParameter *parameter_ = nullptr;
  juce::NormalisableRange<float> range_{0.0f, 1.0f};
  std::atomic<float> cachedValue_{0.0f};
  float defaultValue_ = 0.5f;

  // Appearance
  SkColor accentColor_ = SkColorSetRGB(0, 255, 255); // Cyan default
  juce::String tooltipText_;
  juce::String textSuffix_;

  // Interaction state
  bool isHovered_ = false;
  bool isDragging_ = false;
  bool isFineMode_ = false;

  // Behavior flags
  bool showValueOnHover_ = true;
  bool showValueWhileDragging_ = true;
  bool scrollWheelEnabled_ = true;
  bool doubleClickToReset_ = true;
  float fineControlMultiplier_ = 0.1f;

  // ----- Mouse Handlers -----
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  void mouseWheelMove(const juce::MouseEvent &e,
                      const juce::MouseWheelDetails &wheel) override;
  void modifierKeysChanged(const juce::ModifierKeys &modifiers) override;

  // ----- Helpers -----
  float constrainValue(float value) const;
  void notifyValueChange();

  // Drag state
  float dragStartValue_ = 0.0f;
  juce::Point<float> dragStartPos_;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithControl)
};

} // namespace zenith
