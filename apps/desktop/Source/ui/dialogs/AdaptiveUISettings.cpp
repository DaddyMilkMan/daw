/*
  ==============================================================================

    AdaptiveUISettings.cpp
    Created: 2025-12-07
    Author:  AI Assistant

    Settings implementation for AI-Powered Adaptive Interface

  ==============================================================================
*/

#include "AdaptiveUISettings.h"

#include "../framework/ConfigurationManager.h"

namespace zenith {
namespace settings {

AdaptiveUISettings::AdaptiveUISettings() {
  setName("AdaptiveUISettings");
  createUI();
  loadSettings();
}

AdaptiveUISettings::~AdaptiveUISettings() { saveSettings(); }

void AdaptiveUISettings::createUI() {
  // Title
  titleLabel_ = std::make_unique<SkiaLabel>();
  titleLabel_->setText("AI-Powered Adaptive Interface",
                       juce::dontSendNotification);
  titleLabel_->setFont(18.0f);
  titleLabel_->setTextColour(design::colors::TEXT_PRIMARY);
  addAndMakeVisible(titleLabel_.get());

  // Enable/disable toggle
  enabledLabel_ = std::make_unique<SkiaLabel>();
  enabledLabel_->setText("Enable Adaptive UI", juce::dontSendNotification);
  enabledLabel_->setFont(design::typography::FONT_MD);
  enabledLabel_->setTextColour(design::colors::TEXT_PRIMARY);
  addAndMakeVisible(enabledLabel_.get());

  enabledButton_ = std::make_unique<SkiaButton>();
  enabledButton_->setText("ON");
  enabledButton_->setStyle(SkiaButton::Style::Primary);
  enabledButton_->setToggleable(true);
  enabledButton_->onClick = [this]() {
    adaptiveUIEnabled_ = enabledButton_->getToggleState();
    updateButtonStates();
    config::ConfigurationManager::getInstance().setBool(
        config::keys::AI_AUTO_SUGGESTIONS, adaptiveUIEnabled_);
  };
  addAndMakeVisible(enabledButton_.get());

  // Learning rate
  learningRateLabel_ = std::make_unique<SkiaLabel>();
  learningRateLabel_->setText("Learning Rate", juce::dontSendNotification);
  learningRateLabel_->setFont(design::typography::FONT_MD);
  learningRateLabel_->setTextColour(design::colors::TEXT_PRIMARY);
  addAndMakeVisible(learningRateLabel_.get());

  learningRateCombo_ = std::make_unique<SkiaComboBox>();
  learningRateCombo_->addItem("Slow (Conservative)", 1);
  learningRateCombo_->addItem("Medium (Balanced)", 2);
  learningRateCombo_->addItem("Fast (Aggressive)", 3);
  learningRateCombo_->setSelectedId(2); // Default to medium
  learningRateCombo_->onChange = [this]() {
    int selectedId = learningRateCombo_->getSelectedId();
    if (selectedId == 1)
      learningRate_ = 0.3f;
    else if (selectedId == 2)
      learningRate_ = 0.5f;
    else if (selectedId == 3)
      learningRate_ = 0.7f;

    config::ConfigurationManager::getInstance().setFloat(
        "adaptiveUI.learningRate", learningRate_);
  };
  addAndMakeVisible(learningRateCombo_.get());

  // Adaptation speed
  adaptationSpeedLabel_ = std::make_unique<SkiaLabel>();
  adaptationSpeedLabel_->setText("Adaptation Speed",
                                 juce::dontSendNotification);
  adaptationSpeedLabel_->setFont(design::typography::FONT_MD);
  adaptationSpeedLabel_->setTextColour(design::colors::TEXT_PRIMARY);
  addAndMakeVisible(adaptationSpeedLabel_.get());

  adaptationSpeedCombo_ = std::make_unique<SkiaComboBox>();
  adaptationSpeedCombo_->addItem("Gradual", 1);
  adaptationSpeedCombo_->addItem("Normal", 2);
  adaptationSpeedCombo_->addItem("Quick", 3);
  adaptationSpeedCombo_->setSelectedId(2); // Default to normal
  adaptationSpeedCombo_->onChange = [this]() {
    int selectedId = adaptationSpeedCombo_->getSelectedId();
    if (selectedId == 1)
      adaptationSpeed_ = 0.2f;
    else if (selectedId == 2)
      adaptationSpeed_ = 0.3f;
    else if (selectedId == 3)
      adaptationSpeed_ = 0.5f;

    config::ConfigurationManager::getInstance().setFloat(
        "adaptiveUI.adaptationSpeed", adaptationSpeed_);
  };
  addAndMakeVisible(adaptationSpeedCombo_.get());

  // Preferred layout
  layoutLabel_ = std::make_unique<SkiaLabel>();
  layoutLabel_->setText("Preferred Layout", juce::dontSendNotification);
  layoutLabel_->setFont(design::typography::FONT_MD);
  layoutLabel_->setTextColour(design::colors::TEXT_PRIMARY);
  addAndMakeVisible(layoutLabel_.get());

  layoutCombo_ = std::make_unique<SkiaComboBox>();
  layoutCombo_->addItem("Default", 1);
  layoutCombo_->addItem("Minimal", 2);
  layoutCombo_->addItem("Advanced", 3);
  layoutCombo_->addItem("Custom", 4);
  layoutCombo_->setSelectedId(1); // Default to default
  layoutCombo_->onChange = [this]() {
    int selectedId = layoutCombo_->getSelectedId();
    if (selectedId == 1)
      preferredLayout_ = "Default";
    else if (selectedId == 2)
      preferredLayout_ = "Minimal";
    else if (selectedId == 3)
      preferredLayout_ = "Advanced";
    else if (selectedId == 4)
      preferredLayout_ = "Custom";

    config::ConfigurationManager::getInstance().setString(
        "adaptiveUI.preferredLayout", preferredLayout_);
  };
  addAndMakeVisible(layoutCombo_.get());

  // Reset button
  resetButton_ = std::make_unique<SkiaButton>();
  resetButton_->setText("Reset Learning Data");
  resetButton_->setStyle(SkiaButton::Style::Secondary);
  resetButton_->onClick = [this]() { resetLearningData(); };
  addAndMakeVisible(resetButton_.get());

  // Apply button
  applyButton_ = std::make_unique<SkiaButton>();
  applyButton_->setText("Apply Settings");
  applyButton_->setStyle(SkiaButton::Style::Primary);
  applyButton_->onClick = [this]() { saveSettings(); };
  addAndMakeVisible(applyButton_.get());
}

void AdaptiveUISettings::resized() {
  auto bounds = getLocalBounds();

  // Title at top
  titleLabel_->setBounds(bounds.removeFromTop(40).reduced(10, 5));

  // Enable/disable toggle
  auto enabledRow = bounds.removeFromTop(40);
  enabledLabel_->setBounds(enabledRow.removeFromLeft(150).reduced(10, 5));
  enabledButton_->setBounds(enabledRow.removeFromLeft(80).reduced(10, 5));

  // Learning rate
  auto learningRateRow = bounds.removeFromTop(40);
  learningRateLabel_->setBounds(
      learningRateRow.removeFromLeft(150).reduced(10, 5));
  learningRateCombo_->setBounds(
      learningRateRow.removeFromLeft(200).reduced(10, 5));

  // Adaptation speed
  auto adaptationSpeedRow = bounds.removeFromTop(40);
  adaptationSpeedLabel_->setBounds(
      adaptationSpeedRow.removeFromLeft(150).reduced(10, 5));
  adaptationSpeedCombo_->setBounds(
      adaptationSpeedRow.removeFromLeft(200).reduced(10, 5));

  // Preferred layout
  auto layoutRow = bounds.removeFromTop(40);
  layoutLabel_->setBounds(layoutRow.removeFromLeft(150).reduced(10, 5));
  layoutCombo_->setBounds(layoutRow.removeFromLeft(200).reduced(10, 5));

  // Buttons at bottom
  auto buttonRow = bounds.removeFromBottom(50);
  resetButton_->setBounds(buttonRow.removeFromLeft(150).reduced(10, 10));
  buttonRow.removeFromLeft(20);
  applyButton_->setBounds(buttonRow.removeFromLeft(100).reduced(10, 10));
}

void AdaptiveUISettings::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  // Draw background
  SkPaint bgPaint;
  bgPaint.setColor(design::colors::BG_DARKER);
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                   bgPaint);

  // Draw border
  SkPaint borderPaint;
  borderPaint.setColor(design::colors::BORDER_DEFAULT);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                   borderPaint);
}

void AdaptiveUISettings::loadSettings() {
  adaptiveUIEnabled_ = config::ConfigurationManager::getInstance().getBool(
      config::keys::AI_AUTO_SUGGESTIONS, true);

  learningRate_ = config::ConfigurationManager::getInstance().getFloat(
      "adaptiveUI.learningRate", 0.5f);

  adaptationSpeed_ = config::ConfigurationManager::getInstance().getFloat(
      "adaptiveUI.adaptationSpeed", 0.3f);

  preferredLayout_ = config::ConfigurationManager::getInstance().getString(
      "adaptiveUI.preferredLayout", "Default");

  updateButtonStates();

  // Update combo boxes
  if (learningRate_ <= 0.35f)
    learningRateCombo_->setSelectedId(1);
  else if (learningRate_ <= 0.65f)
    learningRateCombo_->setSelectedId(2);
  else
    learningRateCombo_->setSelectedId(3);

  if (adaptationSpeed_ <= 0.25f)
    adaptationSpeedCombo_->setSelectedId(1);
  else if (adaptationSpeed_ <= 0.4f)
    adaptationSpeedCombo_->setSelectedId(2);
  else
    adaptationSpeedCombo_->setSelectedId(3);

  if (preferredLayout_ == "Default")
    layoutCombo_->setSelectedId(1);
  else if (preferredLayout_ == "Minimal")
    layoutCombo_->setSelectedId(2);
  else if (preferredLayout_ == "Advanced")
    layoutCombo_->setSelectedId(3);
  else
    layoutCombo_->setSelectedId(4);
}

void AdaptiveUISettings::saveSettings() {
  config::ConfigurationManager::getInstance().setBool(
      config::keys::AI_AUTO_SUGGESTIONS, adaptiveUIEnabled_);

  config::ConfigurationManager::getInstance().setFloat(
      "adaptiveUI.learningRate", learningRate_);

  config::ConfigurationManager::getInstance().setFloat(
      "adaptiveUI.adaptationSpeed", adaptationSpeed_);

  config::ConfigurationManager::getInstance().setString(
      "adaptiveUI.preferredLayout", preferredLayout_);
}

void AdaptiveUISettings::updateButtonStates() {
  if (enabledButton_) {
    enabledButton_->setToggleState(adaptiveUIEnabled_);
    enabledButton_->setText(adaptiveUIEnabled_ ? "ON" : "OFF");
  }
}

void AdaptiveUISettings::resetLearningData() {
  // Reset AI learning data
  config::ConfigurationManager::getInstance().setString(
      "adaptiveUI.learningData", "");

  // Show confirmation
  if (auto *parent = getParentComponent()) {
    // Could show an alert here
  }
}

void AdaptiveUISettings::setAdaptiveUIEnabled(bool enabled) {
  adaptiveUIEnabled_ = enabled;
  updateButtonStates();
  config::ConfigurationManager::getInstance().setBool(
      config::keys::AI_AUTO_SUGGESTIONS, enabled);
}

void AdaptiveUISettings::setLearningRate(float rate) {
  learningRate_ = juce::jlimit(0.1f, 1.0f, rate);
  config::ConfigurationManager::getInstance().setFloat(
      "adaptiveUI.learningRate", learningRate_);
}

void AdaptiveUISettings::setAdaptationSpeed(float speed) {
  adaptationSpeed_ = juce::jlimit(0.1f, 1.0f, speed);
  config::ConfigurationManager::getInstance().setFloat(
      "adaptiveUI.adaptationSpeed", adaptationSpeed_);
}

void AdaptiveUISettings::setPreferredLayout(const juce::String &layout) {
  preferredLayout_ = layout;
  config::ConfigurationManager::getInstance().setString(
      "adaptiveUI.preferredLayout", layout);
}

} // namespace settings
} // namespace zenith