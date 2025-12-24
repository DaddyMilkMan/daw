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

  ==============================================================================
*/

#pragma once

#include "Engine.h"
#include "Settings.h"
#include "../engine/PluginHost.h"
#include "SkiaButton.h"
#include "SkiaComponent.h"
#include "SkiaSlider.h"
#include "widgets/SkiaTextEditor.h"
#include "../network/SecureKeyStore.h"
#include "ZenithDesignSystem.h"
#include <include/core/SkColor.h>

#include <juce_audio_utils/juce_audio_utils.h>

namespace zenith {

//==============================================================================
// Settings Tab Base Class
//==============================================================================
class SettingsTab : public SkiaComponent {
public:
  SettingsTab() = default;
  ~SettingsTab() override = default;

  void drawSkia(SkCanvas *canvas) override {
    // Default background for content area
    canvas->clear(SK_ColorTRANSPARENT);
  }
};

//==============================================================================
// Audio Settings Tab
//==============================================================================
class AudioSettingsTab : public SettingsTab {
public:
  AudioSettingsTab(Engine &engine) : engine_(engine) {
    // Setup Button
    setupButton_ = std::make_unique<SkiaButton>("Configure Audio Device...");
    setupButton_->setStyle(SkiaButton::Style::Primary);
    setupButton_->onClick = [this]() { showDeviceSelector(); };
    addAndMakeVisible(setupButton_.get());

    // Refresh timer for status
    startTimer(500);
  }

  void resized() override {
    if (setupButton_) {
      setupButton_->setBounds(20, 180, 200, 36);
    }
  }

  void timerCallback() override { markDirty(); }

  void drawSkia(SkCanvas *canvas) override {
    auto *device = engine_.getDeviceManager().getCurrentAudioDevice();

    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);

    SkFont headerFont;
    headerFont.setSize(24.0f);
    headerFont.setEmbolden(true);

    SkFont labelFont;
    labelFont.setSize(14.0f);
    labelFont.setEmbolden(true);

    SkFont valueFont;
    valueFont.setSize(14.0f);

    // Header
    canvas->drawString("Audio Settings", 20, 40, headerFont, textPaint);

    if (device) {
      float y = 80;
      float labelX = 20;
      float valueX = 140;
      float rowH = 30;

      auto drawRow = [&](const char *label, juce::String value) {
        textPaint.setColor(SkColorSetARGB(180, 255, 255, 255));
        canvas->drawString(label, labelX, y, labelFont, textPaint);

        textPaint.setColor(SK_ColorWHITE);
        canvas->drawString(value.toStdString().c_str(), valueX, y, valueFont,
                           textPaint);
        y += rowH;
      };

      drawRow("Device Type:", device->getTypeName());
      drawRow("Device Name:", device->getName());
      drawRow("Sample Rate:",
              juce::String(device->getCurrentSampleRate()) + " Hz");
      drawRow("Buffer Size:",
              juce::String(device->getCurrentBufferSizeSamples()) + " samples");
    } else {
      textPaint.setColor(SkColorSetARGB(255, 255, 100, 100)); // Red
      canvas->drawString("No Audio Device Selected", 20, 80, valueFont,
                         textPaint);
    }
  }

private:
  void showDeviceSelector() {
    juce::DialogWindow::LaunchOptions options;
    auto *content = new juce::AudioDeviceSelectorComponent(
        engine_.getDeviceManager(), 0, 256, 0, 256, false, false, false, false);
    content->setSize(500, 450);

    options.content.setOwned(content);
    options.dialogTitle = "Audio Device Configuration";
    options.dialogBackgroundColour = juce::Colours::black;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
  }

  Engine &engine_;
  std::unique_ptr<SkiaButton> setupButton_;
};

//==============================================================================
// Display Settings Tab
//==============================================================================
class DisplaySettingsTab : public SettingsTab {
public:
  DisplaySettingsTab() {
    // FPS Slider
    fpsSlider_ = std::make_unique<SkiaSlider>("Target FPS");
    fpsSlider_->setStyle(SkiaSlider::Style::Bar);
    fpsSlider_->setDisplayRange(30, 240);
    fpsSlider_->setValue((float)Settings::getInstance().getTargetFPS());
    fpsSlider_->onValueChange = [](float v) {
      Settings::getInstance().setTargetFPS((int)v);
    };
    addAndMakeVisible(fpsSlider_.get());

    // Glow Slider
    glowSlider_ = std::make_unique<SkiaSlider>("Glow Intensity");
    glowSlider_->setStyle(SkiaSlider::Style::Bar);
    glowSlider_->setDisplayRange(0.0f, 2.0f);
    glowSlider_->setValue(Settings::getInstance().getGlowIntensity());
    glowSlider_->onValueChange = [](float v) {
      Settings::getInstance().setGlowIntensity(v);
    };
    addAndMakeVisible(glowSlider_.get());
  }

  void resized() override {
    fpsSlider_->setBounds(20, 80, 300, 30);
    glowSlider_->setBounds(20, 150, 300, 30);
  }

  void drawSkia(SkCanvas *canvas) override {
    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);

    SkFont headerFont;
    headerFont.setSize(24.0f);
    headerFont.setEmbolden(true);

    SkFont labelFont;
    labelFont.setSize(14.0f);

    canvas->drawString("Display Settings", 20, 40, headerFont, textPaint);

    canvas->drawString("Target FPS", 20, 70, labelFont, textPaint);
    canvas->drawString("Neon Glow Intensity", 20, 140, labelFont, textPaint);
  }

private:
  std::unique_ptr<SkiaSlider> fpsSlider_;
  std::unique_ptr<SkiaSlider> glowSlider_;
};

//==============================================================================
// Plugins Settings Tab
//==============================================================================
class PluginSettingsTab : public SettingsTab {
public:
  PluginSettingsTab(PluginHost &host) : host_(host) {
    scanButton_ = std::make_unique<SkiaButton>("Scan Plugins");
    scanButton_->setStyle(SkiaButton::Style::Success);
    scanButton_->onClick = [this]() { startScan(); };
    addAndMakeVisible(scanButton_.get());

    // Simple text editor for paths (standard JUCE for now, styled later)
    pathList_.setMultiLine(true);
    pathList_.setReadOnly(true);
    pathList_.setColour(juce::TextEditor::backgroundColourId,
                        juce::Colours::transparentBlack);
    pathList_.setColour(juce::TextEditor::outlineColourId,
                        juce::Colours::white.withAlpha(0.2f));
    addAndMakeVisible(pathList_);

    updateList();
  }

  void resized() override {
    pathList_.setBounds(20, 80, getWidth() - 40, getHeight() - 140);
    scanButton_->setBounds(getWidth() - 140, getHeight() - 50, 120, 36);
  }

  void drawSkia(SkCanvas *canvas) override {
    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);

    SkFont headerFont;
    headerFont.setSize(24.0f);
    headerFont.setEmbolden(true);

    SkFont labelFont;
    labelFont.setSize(14.0f);

    canvas->drawString("Plugin Management", 20, 40, headerFont, textPaint);
    canvas->drawString("Search Paths:", 20, 70, labelFont, textPaint);
  }

private:
  void startScan() {
    scanButton_->setText("Scanning...");
    // Fake async scan call for UI update
    host_.scanAsync([this](int p, int c, const juce::String &m) {
      if (p >= 100)
        scanButton_->setText("Scan Plugins");
    });
  }

  void updateList() {
    juce::String text;
    for (const auto &path : host_.getSearchPaths()) {
      text += path + "\n";
    }
    pathList_.setText(text);
  }

  PluginHost &host_;
  std::unique_ptr<SkiaButton> scanButton_;
  juce::TextEditor pathList_;
};

//==============================================================================
// AI Settings Tab
//==============================================================================
class AISettingsTab : public SettingsTab {
public:
  AISettingsTab() {
    // API Key Editor
    apiKeyEditor_ = std::make_unique<SkiaTextEditor>("GrokAPIKey");
    apiKeyEditor_->setTextToShowWhenEmpty("Paste your x.ai API key here...", 
                                         SkColorSetARGB(150, 255, 255, 255));
    addAndMakeVisible(apiKeyEditor_.get());

    // Load existing key (masked for security)
    juce::String existingKey;
    if (SecureKeyStore::retrieveKey(SecureKeyStore::GrokAPIKey, existingKey)) {
      apiKeyEditor_->setText("********" + existingKey.suffix(4));
    }

    // Save Button
    saveButton_ = std::make_unique<SkiaButton>("Save API Key");
    saveButton_->setStyle(SkiaButton::Style::Primary);
    saveButton_->onClick = [this]() { saveKey(); };
    addAndMakeVisible(saveButton_.get());

    // Clear Button
    clearButton_ = std::make_unique<SkiaButton>("Clear");
    clearButton_->setStyle(SkiaButton::Style::Danger);
    clearButton_->onClick = [this]() { clearKey(); };
    addAndMakeVisible(clearButton_.get());
  }

  void resized() override {
    apiKeyEditor_->setBounds(20, 100, getWidth() - 40, 36);
    saveButton_->setBounds(20, 150, 140, 36);
    clearButton_->setBounds(170, 150, 100, 36);
  }

  void drawSkia(SkCanvas *canvas) override {
    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);

    SkFont headerFont;
    headerFont.setSize(24.0f);
    headerFont.setEmbolden(true);

    SkFont labelFont;
    labelFont.setSize(14.0f);

    canvas->drawString("AI Assistant Settings", 20, 40, headerFont, textPaint);
    canvas->drawString("Grok 4.1 API Key (from console.x.ai):", 20, 85, labelFont, textPaint);
    
    if (showSavedMsg_) {
        textPaint.setColor(design::colors::NEON_GREEN);
        canvas->drawString("API Key Saved Successfully!", 20, 210, labelFont, textPaint);
    }
  }

private:
  void saveKey() {
    juce::String key = apiKeyEditor_->getText().trim();
    if (key.contains("*")) return; // Don't save the masked version

    if (key.isNotEmpty()) {
      SecureKeyStore::storeKey(SecureKeyStore::GrokAPIKey, key);
      apiKeyEditor_->setText("********" + key.suffix(4));
      
      showSavedMsg_ = true;
      juce::Timer::callAfterDelay(3000, [this]() {
          showSavedMsg_ = false;
          markDirty();
      });
      markDirty();
    }
  }

  void clearKey() {
    SecureKeyStore::deleteKey(SecureKeyStore::GrokAPIKey);
    apiKeyEditor_->setText("");
    markDirty();
  }

  std::unique_ptr<SkiaTextEditor> apiKeyEditor_;
  std::unique_ptr<SkiaButton> saveButton_;
  std::unique_ptr<SkiaButton> clearButton_;
  bool showSavedMsg_ = false;
};

//==============================================================================
// Main Settings Component
//==============================================================================
class SettingsComponent : public SkiaComponent {
public:
  SettingsComponent(Engine &engine) : engine_(engine) {
    setSize(800, 600);

    // Create Tabs
    audioTab_ = std::make_unique<AudioSettingsTab>(engine);
    addChildComponent(audioTab_.get());

    displayTab_ = std::make_unique<DisplaySettingsTab>();
    addChildComponent(displayTab_.get());

    pluginTab_ = std::make_unique<PluginSettingsTab>(engine.getPluginHost());
    addChildComponent(pluginTab_.get());

    aiTab_ = std::make_unique<AISettingsTab>();
    addChildComponent(aiTab_.get());

    // Create Sidebar Buttons
    createNavButton("Audio", 0);
    createNavButton("Display", 1);
    createNavButton("Plugins", 2);
    createNavButton("AI", 3);

    setActiveTab(0);
  }

  ~SettingsComponent() override = default;

  void resized() override {
    int sidebarWidth = 200;
    int btnHeight = 40;
    int y = 20;

    for (auto *btn : navButtons_) {
      btn->setBounds(10, y, sidebarWidth - 20, btnHeight);
      y += btnHeight + 5;
    }

    auto contentArea =
        getLocalBounds().removeFromRight(getWidth() - sidebarWidth);
    if (currentTab_)
      currentTab_->setBounds(contentArea);
  }

  void drawSkia(SkCanvas *canvas) override {
    auto bounds = getLocalBounds();
    int sidebarWidth = 200;

    // Background
    canvas->clear(SkColorSetRGB(20, 20, 25));

    // Sidebar Background
    SkPaint sidebarPaint;
    sidebarPaint.setColor(SkColorSetRGB(30, 30, 35));
    canvas->drawRect(
        SkRect::MakeWH((float)sidebarWidth, (float)bounds.getHeight()),
        sidebarPaint);

    // Vertical Divider
    SkPaint linePaint;
    linePaint.setColor(SkColorSetARGB(50, 255, 255, 255));
    canvas->drawLine((float)sidebarWidth, 0, (float)sidebarWidth,
                     (float)bounds.getHeight(), linePaint);
  }

private:
  void createNavButton(const juce::String &name, int index) {
    auto btn = std::make_unique<SkiaButton>(name);
    btn->setStyle(SkiaButton::Style::Ghost);
    btn->setToggleable(true);
    btn->onClick = [this, index]() { setActiveTab(index); };
    addAndMakeVisible(btn.get());
    navButtons_.add(btn.release());
  }

  void setActiveTab(int index) {
    // Update Buttons
    for (int i = 0; i < navButtons_.size(); ++i) {
      navButtons_[i]->setToggleState(i == index);
      // Highlight active
      navButtons_[i]->setStyle(i == index ? SkiaButton::Style::Secondary
                                          : SkiaButton::Style::Ghost);
    }

    // Switch Content
    if (currentTab_)
      currentTab_->setVisible(false);

    switch (index) {
    case 0:
      currentTab_ = audioTab_.get();
      break;
    case 1:
      currentTab_ = displayTab_.get();
      break;
    case 2:
      currentTab_ = pluginTab_.get();
      break;
    case 3:
      currentTab_ = aiTab_.get();
      break;
    }

    if (currentTab_) {
      currentTab_->setVisible(true);
      resized(); // Re-layout content
    }
  }

  Engine &engine_;
  juce::OwnedArray<SkiaButton> navButtons_;

  std::unique_ptr<AudioSettingsTab> audioTab_;
  std::unique_ptr<DisplaySettingsTab> displayTab_;
  std::unique_ptr<PluginSettingsTab> pluginTab_;
  std::unique_ptr<AISettingsTab> aiTab_;

  SkiaComponent *currentTab_ = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)
};

} // namespace zenith