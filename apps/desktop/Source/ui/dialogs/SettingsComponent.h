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
#include "../engine/PluginHost.h"
#include "../controls/SkiaComboBox.h"
#include "../controls/SkiaLabel.h"
#include "Settings.h"
#include "SkiaComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ZenithTheme.h"
#include <include/core/SkColor.h>
#include "HardwareControlPanel.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include "../network/SecureKeyStore.h"
#include "../network/GrokAPIClient.h"

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
    juce::TextEditor pathList_;
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

    std::unique_ptr<juce::TextEditor> apiKeyEditor_;
    std::unique_ptr<SkiaButton> validateButton_;
    std::unique_ptr<SkiaLabel> helpLabel_;
    std::unique_ptr<SkiaLabel> statusLabel_;
    std::unique_ptr<GrokAPIClient> testClient_;
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
    std::unique_ptr<PluginSettingsTab> pluginTab_;
    std::unique_ptr<AISettingsTab> aiTab_;
    std::unique_ptr<HardwareControlPanel> hardwareTab_;

    SkiaComponent* currentTab_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)
};

} // namespace zenith