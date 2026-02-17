/*
  ==============================================================================

    SettingsComponent.h
    Created: 2025-12-07
    Author:  Zenith DAW Team

    Flagship Settings Panel with Skia rendering.
    Features:
    - Sidebar navigation with glassmorphism
    - Clean, modern typography
    - Hardware-accelerated controls
    - Legacy audio integration wrapped in modern UI

    Implementation in SettingsComponent.cpp

  ==============================================================================
*/

#pragma once

#include "../controls/SkiaButton.h"
#include "../controls/SkiaSlider.h"
#include "../controls/SkiaTextEditor.h"
#include "../engine/PluginHost.h"
#include "../controls/SkiaAlertWindow.h"
#include "../controls/SkiaComboBox.h"
#include "../controls/SkiaFileChooser.h"
#include "../controls/SkiaLabel.h"
#include "Settings.h"
#include "SkiaComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ZenithTheme.h"
#include <include/core/SkColor.h>
#include "HardwareControlPanel.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include "../../network/SecureKeyStore.h"
#include "../../network/GrokDAWClient.h"

namespace zenith {

//==============================================================================
// Settings Tab Base Class
//==============================================================================
class SettingsTab : public SkiaComponent {
public:
    SettingsTab() = default;
    ~SettingsTab() override = default;
    void drawSkia(SkCanvas* canvas) override;
};

//==============================================================================
// Audio Settings Tab
//==============================================================================
class AudioSettingsTab : public SettingsTab {
public:
    explicit AudioSettingsTab(Engine& engine);
    void resized() override;
    void timerCallback() override;
    void drawSkia(SkCanvas* canvas) override;

private:
    void showDeviceSelector();

    Engine& engine_;
    std::unique_ptr<SkiaButton> setupButton_;
    std::unique_ptr<SkiaComboBox> backendSelector_;
    std::unique_ptr<SkiaLabel> backendLabel_;
    std::unique_ptr<SkiaComboBox> bufferSizeSelector_;
    std::unique_ptr<SkiaLabel> bufferLabel_;
    std::unique_ptr<SkiaButton> pdcToggle_;
    std::unique_ptr<SkiaButton> monitoringToggle_;
    std::unique_ptr<SkiaSlider> monitoringVolumeSlider_;
    std::unique_ptr<SkiaAlertWindow> deviceSelectorAlert_;
};

//==============================================================================
// Display Settings Tab
//==============================================================================
class DisplaySettingsTab : public SettingsTab {
public:
    DisplaySettingsTab();
    void resized() override;
    void drawSkia(SkCanvas* canvas) override;

private:
    std::unique_ptr<SkiaSlider> fpsSlider_;
    std::unique_ptr<SkiaSlider> glowSlider_;
    std::unique_ptr<SkiaComboBox> themeSelector_;
    std::unique_ptr<SkiaButton> animationsToggle_;
    std::unique_ptr<SkiaButton> highContrastToggle_;
    std::unique_ptr<SkiaComboBox> meterBallisticsSelector_;
    std::unique_ptr<SkiaSlider> peakHoldSlider_;
};

//==============================================================================
// Recording Settings Tab
//==============================================================================
class RecordingSettingsTab : public SettingsTab {
public:
    RecordingSettingsTab();
    void resized() override;
    void drawSkia(SkCanvas* canvas) override;

private:
    std::unique_ptr<SkiaSlider> countInSlider_;
    std::unique_ptr<SkiaButton> metronomeCountInToggle_;
    std::unique_ptr<SkiaComboBox> bitDepthSelector_;
    std::unique_ptr<SkiaComboBox> fileTypeSelector_;
    std::unique_ptr<SkiaButton> tempoLockToggle_;
};

//==============================================================================
// MIDI Settings Tab
//==============================================================================
class MIDISettingsTab : public SettingsTab {
public:
    MIDISettingsTab();
    void resized() override;
    void drawSkia(SkCanvas* canvas) override;

private:
    std::unique_ptr<SkiaButton> midiThroughToggle_;
    std::unique_ptr<SkiaButton> midiClockOutToggle_;
    std::unique_ptr<SkiaButton> mtcInToggle_;
    std::unique_ptr<SkiaSlider> latencyCompSlider_;
};

//==============================================================================
// Editing Settings Tab
//==============================================================================
class EditingSettingsTab : public SettingsTab {
public:
    EditingSettingsTab();
    void resized() override;
    void drawSkia(SkCanvas* canvas) override;

private:
    std::unique_ptr<SkiaSlider> crossfadeSlider_;
    std::unique_ptr<SkiaButton> snapToggle_;
    std::unique_ptr<SkiaButton> linkSelectionToggle_;
};

//==============================================================================
// Project Settings Tab
//==============================================================================
class ProjectSettingsTab : public SettingsTab {
public:
    ProjectSettingsTab();
    void resized() override;
    void drawSkia(SkCanvas* canvas) override;

private:
    std::unique_ptr<SkiaButton> autoSaveToggle_;
    std::unique_ptr<SkiaSlider> autoSaveIntervalSlider_;
    std::unique_ptr<SkiaSlider> undoHistorySlider_;
    std::unique_ptr<SkiaButton> projectFolderButton_;
    std::unique_ptr<SkiaFileChooser> folderChooser_;
    juce::String currentProjectFolder_;
};

//==============================================================================
// Plugins Settings Tab
//==============================================================================
class PluginSettingsTab : public SettingsTab {
public:
    explicit PluginSettingsTab(PluginHost& host);
    void resized() override;
    void drawSkia(SkCanvas* canvas) override;

private:
    void startScan();
    void updateList();

    PluginHost& host_;
    std::unique_ptr<SkiaButton> scanButton_;
    std::unique_ptr<SkiaTextEditor> pathList_;
};

//==============================================================================
// AI Settings Tab
//==============================================================================
class AISettingsTab : public SettingsTab {
public:
    AISettingsTab();
    void resized() override;
    void drawSkia(SkCanvas* canvas) override;

private:
    void validateKey();

    std::unique_ptr<SkiaTextEditor> apiKeyEditor_;
    std::unique_ptr<SkiaButton> validateButton_;
    std::unique_ptr<SkiaLabel> helpLabel_;
    std::unique_ptr<SkiaLabel> statusLabel_;
    std::unique_ptr<GrokDAWClient> testClient_;
};

//==============================================================================
// About Tab
//==============================================================================
class AboutTab : public SettingsTab {
public:
    AboutTab();
    void resized() override;
    void drawSkia(SkCanvas* canvas) override;
};

//==============================================================================
// Main Settings Component
//==============================================================================
class SettingsComponent : public SkiaComponent {
public:
    explicit SettingsComponent(Engine& engine);
    ~SettingsComponent() override;

    void resized() override;
    void drawSkia(SkCanvas* canvas) override;

private:
    void createNavButton(const juce::String& name, int index);
    void setActiveTab(int index);

    Engine& engine_;
    juce::OwnedArray<SkiaButton> navButtons_;

    std::unique_ptr<AudioSettingsTab> audioTab_;
    std::unique_ptr<DisplaySettingsTab> displayTab_;
    std::unique_ptr<RecordingSettingsTab> recordingTab_;
    std::unique_ptr<MIDISettingsTab> midiTab_;
    std::unique_ptr<EditingSettingsTab> editingTab_;
    std::unique_ptr<ProjectSettingsTab> projectTab_;
    std::unique_ptr<PluginSettingsTab> pluginTab_;
    std::unique_ptr<AISettingsTab> aiTab_;
    std::unique_ptr<HardwareControlPanel> hardwareTab_;
    std::unique_ptr<AboutTab> aboutTab_;

    SkiaComponent* currentTab_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)
};

} // namespace zenith
