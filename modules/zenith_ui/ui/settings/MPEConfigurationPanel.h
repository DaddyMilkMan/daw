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

#include <memory>
#include <vector>
#include "../../JuceHeader.h"

namespace zenith {

/**
 * MPE Zone Settings
 */
struct MPESettings {
  bool lowerZoneEnabled = false;
  int lowerMasterChannel = 1;
  int lowerMemberChannels = 0;

  bool upperZoneEnabled = false;
  int upperMasterChannel = 16;
  int upperMemberChannels = 0;
};

/**
 * MPE Controller Presets
 */
enum class MPEControllerPreset {
  Custom,
  RoliSeaboardBlock,
  RoliSeaboardRise,
  LinnStrument,
  KBoard,
  Push2
};

/**
 * Callback type for MPE settings changes
 */
class MPEConfigurationPanel;
typedef void (*MPESettingsCallback)(MPEConfigurationPanel* panel, const MPESettings& settings, void* userData);

/**
 * Settings panel component for MPE zone configuration
 * Following the pattern from GlobalSettingsPanel
 */
class MPEConfigurationPanel : public juce::Component,
                              private juce::Button::Listener,
                              private juce::ComboBox::Listener,
                              private juce::Slider::Listener {
public:
  MPEConfigurationPanel();
  ~MPEConfigurationPanel() override;

  void setSettingsCallback(MPESettingsCallback callback, void* userData = nullptr);
  MPESettings getSettings() const { return settings_; }
  void setSettings(const MPESettings& settings);
  void notifySettingsChanged();

  void paint(juce::Graphics& g) override;
  void resized() override;

private:
  void presetChanged();
  void applyPreset(MPEControllerPreset preset);
  bool validateSettings() const;
  void updateUIFromSettings();

  void buttonClicked(juce::Button* button) override;
  void comboBoxChanged(juce::ComboBox* comboBox) override;
  void sliderValueChanged(juce::Slider* slider) override;

  // Child components - using std::unique_ptr as per codebase pattern
  std::unique_ptr<juce::ToggleButton> lowerZoneToggle_;
  std::unique_ptr<juce::ComboBox> lowerMasterChannel_;
  std::unique_ptr<juce::Slider> lowerMemberChannels_;

  std::unique_ptr<juce::ToggleButton> upperZoneToggle_;
  std::unique_ptr<juce::ComboBox> upperMasterChannel_;
  std::unique_ptr<juce::Slider> upperMemberChannels_;

  std::unique_ptr<juce::ComboBox> presetSelector_;
  std::unique_ptr<juce::TextButton> applyButton_;
  std::unique_ptr<juce::TextButton> resetButton_;

  MPESettings settings_;
  MPESettingsCallback onSettingsApply_;
  void* settingsUserData_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MPEConfigurationPanel)
};

} // namespace zenith
