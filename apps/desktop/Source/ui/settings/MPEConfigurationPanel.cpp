/*
  ==============================================================================

    MPEConfigurationPanel.cpp
    Created: 2026-01-29
    Author:  Zenith DAW

    Implementation following codebase patterns from GlobalSettingsPanel.

  ==============================================================================
*/

#include "MPEConfigurationPanel.h"
#include "../design-system/ZenithDesignSystem.h"

namespace zenith {

MPEConfigurationPanel::MPEConfigurationPanel()
    : onSettingsApply_(nullptr),
      settingsUserData_(nullptr) {

  // Lower Zone Toggle
  lowerZoneToggle_ = std::make_unique<juce::ToggleButton>("Lower Zone Enabled");
  addAndMakeVisible(*lowerZoneToggle_);
  lowerZoneToggle_->addListener(this);

  // Lower Zone Master Channel
  lowerMasterChannel_ = std::make_unique<juce::ComboBox>("Lower Master Channel");
  addAndMakeVisible(*lowerMasterChannel_);
  for (int i = 0; i < 16; ++i) {
    lowerMasterChannel_->addItem("Channel " + juce::String(i + 1), i + 1);
  }
  lowerMasterChannel_->setSelectedId(1, juce::dontSendNotification);
  lowerMasterChannel_->addListener(this);

  // Lower Zone Member Channels
  lowerMemberChannels_ = std::make_unique<juce::Slider>("Lower Member Channels");
  addAndMakeVisible(*lowerMemberChannels_);
  lowerMemberChannels_->setRange(0, 15, 1);
  lowerMemberChannels_->setValue(0, juce::dontSendNotification);
  lowerMemberChannels_->setSliderStyle(juce::Slider::IncDecButtons);
  lowerMemberChannels_->addListener(this);

  // Upper Zone Toggle
  upperZoneToggle_ = std::make_unique<juce::ToggleButton>("Upper Zone Enabled");
  addAndMakeVisible(*upperZoneToggle_);
  upperZoneToggle_->addListener(this);

  // Upper Zone Master Channel
  upperMasterChannel_ = std::make_unique<juce::ComboBox>("Upper Master Channel");
  addAndMakeVisible(*upperMasterChannel_);
  for (int i = 0; i < 16; ++i) {
    upperMasterChannel_->addItem("Channel " + juce::String(i + 1), i + 1);
  }
  upperMasterChannel_->setSelectedId(16, juce::dontSendNotification);
  upperMasterChannel_->addListener(this);

  // Upper Zone Member Channels
  upperMemberChannels_ = std::make_unique<juce::Slider>("Upper Member Channels");
  addAndMakeVisible(*upperMemberChannels_);
  upperMemberChannels_->setRange(0, 15, 1);
  upperMemberChannels_->setValue(0, juce::dontSendNotification);
  upperMemberChannels_->setSliderStyle(juce::Slider::IncDecButtons);
  upperMemberChannels_->addListener(this);

  // Preset Selector
  presetSelector_ = std::make_unique<juce::ComboBox>("Controller Preset");
  addAndMakeVisible(*presetSelector_);
  presetSelector_->addItem("Custom", static_cast<int>(MPEControllerPreset::Custom));
  presetSelector_->addSeparator();
  presetSelector_->addItem("ROLI Seaboard Block", static_cast<int>(MPEControllerPreset::RoliSeaboardBlock));
  presetSelector_->addItem("ROLI Seaboard Rise", static_cast<int>(MPEControllerPreset::RoliSeaboardRise));
  presetSelector_->addItem("LinnStrument", static_cast<int>(MPEControllerPreset::LinnStrument));
  presetSelector_->addItem("K-Board", static_cast<int>(MPEControllerPreset::KBoard));
  presetSelector_->addItem("Ableton Push 2", static_cast<int>(MPEControllerPreset::Push2));
  presetSelector_->setSelectedId(1, juce::dontSendNotification);
  presetSelector_->addListener(this);

  // Apply Button
  applyButton_ = std::make_unique<juce::TextButton>("Apply");
  addAndMakeVisible(*applyButton_);
  applyButton_->addListener(this);

  // Reset Button
  resetButton_ = std::make_unique<juce::TextButton>("Reset");
  addAndMakeVisible(*resetButton_);
  resetButton_->addListener(this);

  setSize(400, 350);
}

MPEConfigurationPanel::~MPEConfigurationPanel() {
  // std::unique_ptr handles cleanup automatically
}

void MPEConfigurationPanel::setSettingsCallback(MPESettingsCallback callback, void* userData) {
  onSettingsApply_ = callback;
  settingsUserData_ = userData;
}

void MPEConfigurationPanel::setSettings(const MPESettings& settings) {
  settings_ = settings;
  updateUIFromSettings();
}

void MPEConfigurationPanel::notifySettingsChanged() {
  if (onSettingsApply_ != nullptr) {
    onSettingsApply_(this, settings_, settingsUserData_);
  }
}

void MPEConfigurationPanel::paint(juce::Graphics& g) {
  g.fillAll(design::toJuceColour(design::colors::BG_DARK));
}

void MPEConfigurationPanel::resized() {
  auto area = getLocalBounds().reduced(10);
  if (area.isEmpty()) return;

  const int rowHeight = 30;
  const int sectionGap = 20;

  // Lower Zone Section
  auto lowerZone = area.removeFromTop(rowHeight * 3 + sectionGap);
  lowerZoneToggle_->setBounds(lowerZone.removeFromTop(rowHeight));
  lowerZone.removeFromTop(5);
  auto lowerRow1 = lowerZone.removeFromTop(rowHeight);
  lowerMasterChannel_->setBounds(lowerRow1.removeFromLeft(150));
  lowerRow1.removeFromLeft(10);
  lowerMemberChannels_->setBounds(lowerRow1.removeFromLeft(80));

  area.removeFromTop(sectionGap);

  // Upper Zone Section
  auto upperZone = area.removeFromTop(rowHeight * 3 + sectionGap);
  upperZoneToggle_->setBounds(upperZone.removeFromTop(rowHeight));
  upperZone.removeFromTop(5);
  auto upperRow1 = upperZone.removeFromTop(rowHeight);
  upperMasterChannel_->setBounds(upperRow1.removeFromLeft(150));
  upperRow1.removeFromLeft(10);
  upperMemberChannels_->setBounds(upperRow1.removeFromLeft(80));

  area.removeFromTop(sectionGap);

  // Preset Section
  auto presetArea = area.removeFromTop(rowHeight);
  presetSelector_->setBounds(presetArea);

  area.removeFromTop(sectionGap);

  // Buttons
  auto buttonArea = area.removeFromTop(rowHeight);
  buttonArea.removeFromLeft(150);
  applyButton_->setBounds(buttonArea.removeFromLeft(80));
  buttonArea.removeFromLeft(10);
  resetButton_->setBounds(buttonArea.removeFromLeft(80));
}

void MPEConfigurationPanel::buttonClicked(juce::Button* button) {
  if (button == nullptr) return;

  if (button == lowerZoneToggle_.get()) {
    settings_.lowerZoneEnabled = lowerZoneToggle_->getToggleState();
  } else if (button == upperZoneToggle_.get()) {
    settings_.upperZoneEnabled = upperZoneToggle_->getToggleState();
  } else if (button == applyButton_.get()) {
    if (validateSettings()) {
      notifySettingsChanged();
    }
  } else if (button == resetButton_.get()) {
    MPESettings defaults;
    setSettings(defaults);
  }
}

void MPEConfigurationPanel::comboBoxChanged(juce::ComboBox* comboBox) {
  if (comboBox == nullptr) return;

  if (comboBox == presetSelector_.get()) {
    presetChanged();
  } else if (comboBox == lowerMasterChannel_.get()) {
    settings_.lowerMasterChannel = lowerMasterChannel_->getSelectedId();
  } else if (comboBox == upperMasterChannel_.get()) {
    settings_.upperMasterChannel = upperMasterChannel_->getSelectedId();
  }
}

void MPEConfigurationPanel::sliderValueChanged(juce::Slider* slider) {
  if (slider == nullptr) return;

  if (slider == lowerMemberChannels_.get()) {
    settings_.lowerMemberChannels = static_cast<int>(lowerMemberChannels_->getValue());
  } else if (slider == upperMemberChannels_.get()) {
    settings_.upperMemberChannels = static_cast<int>(upperMemberChannels_->getValue());
  }
}

void MPEConfigurationPanel::presetChanged() {
  if (presetSelector_ == nullptr) return;

  int presetId = presetSelector_->getSelectedId();
  auto preset = static_cast<MPEControllerPreset>(presetId);
  applyPreset(preset);
}

void MPEConfigurationPanel::applyPreset(MPEControllerPreset preset) {
  switch (preset) {
    case MPEControllerPreset::RoliSeaboardBlock:
      settings_.lowerZoneEnabled = true;
      settings_.lowerMasterChannel = 1;
      settings_.lowerMemberChannels = 15;
      settings_.upperZoneEnabled = false;
      break;

    case MPEControllerPreset::RoliSeaboardRise:
      settings_.lowerZoneEnabled = true;
      settings_.lowerMasterChannel = 1;
      settings_.lowerMemberChannels = 15;
      settings_.upperZoneEnabled = false;
      break;

    case MPEControllerPreset::LinnStrument:
      settings_.lowerZoneEnabled = true;
      settings_.lowerMasterChannel = 1;
      settings_.lowerMemberChannels = 15;
      settings_.upperZoneEnabled = false;
      break;

    case MPEControllerPreset::KBoard:
      settings_.lowerZoneEnabled = true;
      settings_.lowerMasterChannel = 1;
      settings_.lowerMemberChannels = 7;
      settings_.upperZoneEnabled = true;
      settings_.upperMasterChannel = 16;
      settings_.upperMemberChannels = 7;
      break;

    case MPEControllerPreset::Push2:
      settings_.lowerZoneEnabled = false;
      settings_.upperZoneEnabled = false;
      break;

    case MPEControllerPreset::Custom:
    default:
      break;
  }

  updateUIFromSettings();
}

bool MPEConfigurationPanel::validateSettings() const {
  if (settings_.lowerMasterChannel < 1 || settings_.lowerMasterChannel > 16) {
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon,
        "Invalid MPE Settings",
        "Lower master channel must be between 1 and 16.",
        "OK"
    );
    return false;
  }

  if (settings_.upperMasterChannel < 1 || settings_.upperMasterChannel > 16) {
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon,
        "Invalid MPE Settings",
        "Upper master channel must be between 1 and 16.",
        "OK"
    );
    return false;
  }

  if (settings_.lowerMemberChannels < 0 || settings_.lowerMemberChannels > 15) {
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon,
        "Invalid MPE Settings",
        "Lower member channels must be between 0 and 15.",
        "OK"
    );
    return false;
  }

  if (settings_.upperMemberChannels < 0 || settings_.upperMemberChannels > 15) {
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon,
        "Invalid MPE Settings",
        "Upper member channels must be between 0 and 15.",
        "OK"
    );
    return false;
  }

  if (settings_.lowerZoneEnabled && settings_.upperZoneEnabled) {
    int lowerEnd = settings_.lowerMasterChannel + settings_.lowerMemberChannels;
    int upperStart = settings_.upperMasterChannel - settings_.upperMemberChannels;

    if (lowerEnd >= upperStart) {
      juce::AlertWindow::showMessageBoxAsync(
          juce::AlertWindow::WarningIcon,
          "Invalid MPE Settings",
          "Lower and Upper zones overlap. Please adjust the master channels or member channel counts.",
          "OK"
      );
      return false;
    }
  }

  return true;
}

void MPEConfigurationPanel::updateUIFromSettings() {
  if (lowerZoneToggle_ == nullptr) return;

  lowerZoneToggle_->setToggleState(settings_.lowerZoneEnabled, juce::dontSendNotification);
  lowerMasterChannel_->setSelectedId(settings_.lowerMasterChannel, juce::dontSendNotification);
  lowerMemberChannels_->setValue(settings_.lowerMemberChannels, juce::dontSendNotification);

  upperZoneToggle_->setToggleState(settings_.upperZoneEnabled, juce::dontSendNotification);
  upperMasterChannel_->setSelectedId(settings_.upperMasterChannel, juce::dontSendNotification);
  upperMemberChannels_->setValue(settings_.upperMemberChannels, juce::dontSendNotification);
}

} // namespace zenith
