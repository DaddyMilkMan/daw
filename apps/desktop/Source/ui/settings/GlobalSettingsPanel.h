/*
  ==============================================================================

    GlobalSettingsPanel.h
    Created: 2025-12-30 / Redesigned: 2026-01-13
    Author:  Zenith DAW

    Professional Settings Panel with:
    - Tabbed navigation (Audio, MIDI, Recording, Editing, Display, General)
    - Searchable settings with filtering
    - Grouped settings with labels and descriptions
    - Reset to defaults functionality
    - Change indicators for modified settings

  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../controls/SkiaComboBox.h"
#include "../controls/ZenithButton.h"
#include "../controls/ZenithToggle.h"
#include "../controls/SkiaTextInput.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <map>
#include <vector>

namespace zenith {

//==============================================================================
// SettingRow - Individual setting with label, description, and control
//==============================================================================

class SettingRow : public SkiaComponent {
public:
  enum class Type { Toggle, Combo, Button, Info };

  SettingRow(const juce::String& id, const juce::String& label, 
             const juce::String& description, Type type);

  void resized() override;
  void drawSkia(SkCanvas* canvas) override;
  void mouseEnter(const juce::MouseEvent& e) override;
  void mouseExit(const juce::MouseEvent& e) override;

  // Identification
  juce::String getId() const { return id_; }
  
  // Search filtering
  bool matchesSearch(const juce::String& query) const;

  // Toggle control
  void setToggleState(bool state);
  bool getToggleState() const;
  std::function<void(bool)> onToggle;

  // Combo control
  void setComboItems(const juce::StringArray& items);
  void setSelectedId(int id);
  int getSelectedId() const;
  juce::String getSelectedText() const;
  std::function<void()> onComboChange;

  // Button control
  void setButtonText(const juce::String& text);
  std::function<void()> onButtonClick;

  // Info display
  void setInfoText(const juce::String& text);

  // Change indicator
  void setChanged(bool changed) { isChanged_ = changed; markDirty(); }
  bool isChanged() const { return isChanged_; }

  // Tooltip
  void setTooltipText(const juce::String& tip) { tooltip_ = tip; }
  juce::String getTooltipText() const { return tooltip_; }

private:
  juce::String id_;
  juce::String label_;
  juce::String description_;
  juce::String tooltip_;
  juce::String infoText_;
  Type type_;
  bool isChanged_ = false;

  std::unique_ptr<ZenithToggle> toggle_;
  std::unique_ptr<SkiaComboBox> combo_;
  std::unique_ptr<ZenithButton> button_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingRow)
};

//==============================================================================
// SettingGroup - Container for related settings with header
//==============================================================================

class SettingGroup : public SkiaComponent {
public:
  explicit SettingGroup(const juce::String& title);

  void addSetting(SettingRow* row);
  void filterSettings(const juce::String& query);
  bool hasVisibleSettings() const;
  int getContentHeight() const;

  void resized() override;
  void drawSkia(SkCanvas* canvas) override;

private:
  juce::String title_;
  std::vector<SettingRow*> rows_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingGroup)
};

//==============================================================================
// GlobalSettingsPanel - Main settings dialog
//==============================================================================

class GlobalSettingsPanel : public SkiaComponent,
                            public juce::ChangeListener {
public:
  explicit GlobalSettingsPanel(juce::AudioDeviceManager& deviceManager);
  ~GlobalSettingsPanel() override;

  void resized() override;
  void drawSkia(SkCanvas* canvas) override;
  void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& d) override;

  // ChangeListener for device changes
  void changeListenerCallback(juce::ChangeBroadcaster* source) override;

  // Keyboard handling
  bool keyPressed(const juce::KeyPress& key) override;

  // Close callback
  std::function<void()> onClose;

  // Tab enumeration
  enum class Tab { Audio, MIDI, Recording, Editing, Display, General };

private:
  juce::AudioDeviceManager& deviceManager_;

  // UI Components
  std::unique_ptr<SkiaTextInput> searchField_;
  std::unique_ptr<ZenithButton> closeBtn_;
  std::unique_ptr<ZenithButton> resetBtn_;
  std::unique_ptr<SkiaComponent> contentContainer_;
  std::vector<std::unique_ptr<ZenithButton>> tabButtons_;

  // Settings organization
  std::map<Tab, std::vector<std::unique_ptr<SettingRow>>> rows_;
  std::map<Tab, std::vector<std::unique_ptr<SettingGroup>>> groups_;

  // State
  Tab currentTab_ = Tab::Audio;
  juce::String searchQuery_;
  float scrollY_ = 0.0f;
  float maxScroll_ = 0.0f;

  // Setup methods
  void createTabs();
  void createAudioSettings();
  void createMidiSettings();
  void createRecordingSettings();
  void createEditingSettings();
  void createDisplaySettings();
  void createGeneralSettings();

  // Row creation helpers
  SettingRow* createToggleRow(Tab tab, const juce::String& id, const juce::String& label,
                              const juce::String& desc, const juce::String& tooltip);
  SettingRow* createComboRow(Tab tab, const juce::String& id, const juce::String& label,
                             const juce::String& desc, const juce::String& tooltip,
                             const juce::StringArray& items);
  SettingRow* createButtonRow(Tab tab, const juce::String& id, const juce::String& label,
                              const juce::String& desc, const juce::String& tooltip,
                              const juce::String& buttonText);
  SettingRow* createInfoRow(Tab tab, const juce::String& id, const juce::String& label,
                            const juce::String& desc);

  // Tab management
  void setCurrentTab(Tab tab);
  void filterSettings(const juce::String& query);
  void resetCurrentTabToDefaults();

  // Settings sync
  void syncWithSettings();
  void refreshAudioDevices();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalSettingsPanel)
};

} // namespace zenith
