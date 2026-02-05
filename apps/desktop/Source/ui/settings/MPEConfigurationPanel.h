/*
  ==============================================================================

    MPEConfigurationPanel.h
    Created: 2026-01-29
    Author:  Zenith DAW

    Settings panel for configuring MPE zones - following codebase patterns.

  ==============================================================================
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
