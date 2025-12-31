/*
  ==============================================================================

    GlobalSettingsPanel.h
    Created: 2025-12-30
    Author:  Zenith DAW

    Premium, Skia-based Global Settings Panel.
    Handles Audio Device, MIDI, and General Preferences.

  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../controls/SkiaComboBox.h"
#include "../controls/SkiaButton.h"
#include <juce_audio_devices/juce_audio_devices.h>

namespace zenith {

class GlobalSettingsPanel : public SkiaComponent,
                            public juce::ChangeListener {
public:
  GlobalSettingsPanel(juce::AudioDeviceManager& deviceManager);
  ~GlobalSettingsPanel() override;

  void resized() override;
  void drawSkia(SkCanvas* canvas) override;
  
  // ChangeListener for device changes
  void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
  juce::AudioDeviceManager& deviceManager_;

  // Tabs / Sections (Simulated for now with layout)
  enum class Section { Audio, MIDI, General };
  Section currentSection_ = Section::Audio;

  // Audio Controls
  std::unique_ptr<SkiaComboBox> outputDeviceCombo_;
  std::unique_ptr<SkiaComboBox> inputDeviceCombo_;
  std::unique_ptr<SkiaComboBox> sampleRateCombo_;
  std::unique_ptr<SkiaComboBox> bufferSizeCombo_;
  std::unique_ptr<SkiaButton> testToneBtn_;

  // MIDI Controls (Placeholder for list)
  // std::unique_ptr<SkiaListBox> midiInputList_; 

  // Actions
  std::unique_ptr<SkiaButton> closeBtn_;

  void refreshAudioDeviceList();
  void updateComboBoxes();
  void applyAudioSettings();

  // Helpers
  void createControls();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalSettingsPanel)
};

} // namespace zenith
