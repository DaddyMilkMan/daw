/*
  ==============================================================================

    GlobalSettingsPanel.h
    Created: 2025-12-30
    Updated: 2026-01-02
    Author:  Zenith DAW

    Premium Settings Modal with modular tab architecture.

  ==============================================================================
*/

#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../framework/SkiaComponent.h"
#include "../controls/ZenithButton.h"
#include "../controls/ZenithToggle.h"
#include "../controls/ZenithSlider.h"
#include "../controls/ZenithTextInput.h"
#include "../controls/SkiaComboBox.h"
#include "../controls/SkiaListBox.h"
#include "../controls/SkiaLabel.h"

namespace zenith {

// Forward declarations for Sub-Panels
class AudioSettingsPanel;
class MidiSettingsPanel;
class PluginSettingsPanel;
class KeyboardSettingsPanel;
class AppearanceSettingsPanel;
class GeneralSettingsPanel;
class CollaborationSettingsPanel;
class AdvancedSettingsPanel;

class GlobalSettingsPanel : public SkiaComponent,
                            public juce::ChangeListener {
public:
  enum class Category {
    General,
    Audio,
    MIDI,
    Plugins,
    Appearance,
    Keyboard,
    Collaboration,
    Advanced,
    COUNT
  };

  struct CategoryInfo {
    const char* name;
    const char* icon; // Placeholder for icon font char
  };

  static constexpr CategoryInfo categories_[] = {
    {"General", "A"},
    {"Audio", "B"},
    {"MIDI", "C"},
    {"Plugins", "D"},
    {"Appearance", "E"},
    {"Keyboard", "F"},
    {"Collaboration", "G"},
    {"Advanced", "H"}
  };

  GlobalSettingsPanel(juce::AudioDeviceManager& deviceManager);
  ~GlobalSettingsPanel() override;

  void resized() override;
  void drawSkia(SkCanvas* canvas) override;
  
  // Modal visibility API
  void show();
  void hide();
  std::function<void()> onClose;
  
  void mouseDown(const juce::MouseEvent& event) override;

private:
  void createTabButtons();
  void switchCategory(Category category);
  
  // JUCE ChangeListener
  void changeListenerCallback(juce::ChangeBroadcaster* source) override;

  juce::AudioDeviceManager& deviceManager_;
  Category currentCategory_ = Category::General;
  
  // Sub-Panels
  std::unique_ptr<GeneralSettingsPanel> generalPanel_;
  std::unique_ptr<AudioSettingsPanel> audioPanel_;
  std::unique_ptr<MidiSettingsPanel> midiPanel_;
  std::unique_ptr<PluginSettingsPanel> pluginPanel_;
  std::unique_ptr<AppearanceSettingsPanel> appearancePanel_;
  std::unique_ptr<KeyboardSettingsPanel> keyboardPanel_;
  std::unique_ptr<CollaborationSettingsPanel> collabPanel_;
  std::unique_ptr<AdvancedSettingsPanel> advancedPanel_;
  
  // Navigation
  std::vector<std::unique_ptr<ZenithButton>> tabButtons_;
  std::unique_ptr<ZenithButton> closeBtn_;

  // Animation state
  bool isVisible_ = false;
  uint32_t canCloseAfter_ = 0; // Bounce guard timestamp

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalSettingsPanel)
};

} // namespace zenith