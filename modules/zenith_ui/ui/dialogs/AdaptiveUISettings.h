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

// AdaptiveUISettings.h


#include "../controls/SkiaButton.h"
#include "../controls/SkiaComboBox.h"
#include "../controls/SkiaLabel.h"
#include "../framework/SkiaComponent.h"

namespace zenith {
namespace settings {

class AdaptiveUISettings : public SkiaComponent {
public:
  AdaptiveUISettings();
  ~AdaptiveUISettings() override;

  // Component interface
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  // Settings
  void setAdaptiveUIEnabled(bool enabled);
  bool isAdaptiveUIEnabled() const { return adaptiveUIEnabled_; }

  void setLearningRate(float rate);
  float getLearningRate() const { return learningRate_; }

  void setAdaptationSpeed(float speed);
  float getAdaptationSpeed() const { return adaptationSpeed_; }

  void setPreferredLayout(const juce::String &layout);
  juce::String getPreferredLayout() const { return preferredLayout_; }

  void resetLearningData();

private:
  // Settings state
  bool adaptiveUIEnabled_ = true;
  float learningRate_ = 0.5f;
  float adaptationSpeed_ = 0.3f;
  juce::String preferredLayout_ = "Default";

  // UI Components
  std::unique_ptr<SkiaLabel> titleLabel_;
  std::unique_ptr<SkiaLabel> enabledLabel_;
  std::unique_ptr<SkiaButton> enabledButton_;

  std::unique_ptr<SkiaLabel> learningRateLabel_;
  std::unique_ptr<SkiaComboBox> learningRateCombo_;

  std::unique_ptr<SkiaLabel> adaptationSpeedLabel_;
  std::unique_ptr<SkiaComboBox> adaptationSpeedCombo_;

  std::unique_ptr<SkiaLabel> layoutLabel_;
  std::unique_ptr<SkiaComboBox> layoutCombo_;

  std::unique_ptr<SkiaButton> resetButton_;
  std::unique_ptr<SkiaButton> applyButton_;

  void createUI();
  void loadSettings();
  void saveSettings();
  void updateButtonStates();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdaptiveUISettings)
};

} // namespace settings
} // namespace zenith