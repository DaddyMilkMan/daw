/*
  ==============================================================================

    AdaptiveUISettings.h
    Created: 2025-12-07
    Author:  AI Assistant

    Settings panel for AI-Powered Adaptive Interface

  ==============================================================================
*/

#pragma once

<<<<<<< HEAD
#include "SkiaButton.h"
#include "SkiaComponent.h"
#include "../controls/SkiaComboBox.h"
#include "../controls/SkiaLabel.h"
=======
#include "../framework/SkiaComponent.h"
#include "../widgets/SkiaButton.h"
#include "../widgets/SkiaComboBox.h"
#include "../widgets/SkiaLabel.h"

>>>>>>> origin/refactor/header-consolidation

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