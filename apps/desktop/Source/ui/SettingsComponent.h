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

#include "../../include/Engine.h"
#include "../Settings.h"
#include "../engine/PluginHost.h"
#include "skia/SkiaButton.h"
#include "skia/SkiaComponent.h"
#include "skia/SkiaSlider.h"
#include "skia/ZenithDesignSystem.h"
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
// Plugins Settings Tab - Enhanced with Progress & Blacklist Management
//==============================================================================
class PluginSettingsTab : public SettingsTab {
public:
  PluginSettingsTab(PluginHost &host) : host_(host) {
    // Scan button
    scanButton_ = std::make_unique<SkiaButton>("Scan Plugins");
    scanButton_->setStyle(SkiaButton::Style::Success);
    scanButton_->onClick = [this]() { startScan(); };
    addAndMakeVisible(scanButton_.get());
    
    // Cancel button (initially hidden)
    cancelButton_ = std::make_unique<SkiaButton>("Cancel");
    cancelButton_->setStyle(SkiaButton::Style::Danger);
    cancelButton_->onClick = [this]() { 
      host_.cancelScan();
      scanButton_->setText("Scan Plugins");
      cancelButton_->setVisible(false);
    };
    addChildComponent(cancelButton_.get());
    
    // Add path button
    addPathButton_ = std::make_unique<SkiaButton>("+ Add Path");
    addPathButton_->setStyle(SkiaButton::Style::Secondary);
    addPathButton_->onClick = [this]() { addCustomPath(); };
    addAndMakeVisible(addPathButton_.get());
    
    // Clear blacklist button
    clearBlacklistButton_ = std::make_unique<SkiaButton>("Clear Blacklist");
    clearBlacklistButton_->setStyle(SkiaButton::Style::Ghost);
    clearBlacklistButton_->onClick = [this]() {
      host_.clearBlacklist();
      markDirty();
    };
    addAndMakeVisible(clearBlacklistButton_.get());

    // Start timer for UI updates during scanning
    startTimer(100);
  }
  
  ~PluginSettingsTab() override {
    stopTimer();
  }

  void resized() override {
    int btnWidth = 120;
    int btnHeight = 36;
    int margin = 20;
    int bottomY = getHeight() - 50;
    
    // Bottom row buttons
    scanButton_->setBounds(getWidth() - margin - btnWidth, bottomY, btnWidth, btnHeight);
    cancelButton_->setBounds(getWidth() - margin - btnWidth * 2 - 10, bottomY, btnWidth, btnHeight);
    addPathButton_->setBounds(margin, bottomY, btnWidth, btnHeight);
    clearBlacklistButton_->setBounds(margin + btnWidth + 10, bottomY, btnWidth + 20, btnHeight);
  }
  
  void timerCallback() override {
    // Call base class for animations
    SkiaComponent::timerCallback();
    
    if (host_.isScanningPlugins()) {
      markDirty();
    }
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
    labelFont.setEmbolden(true);
    
    SkFont valueFont;
    valueFont.setSize(14.0f);
    
    SkFont smallFont;
    smallFont.setSize(12.0f);

    float y = 40;
    
    // Header
    canvas->drawString("Plugin Management", 20, y, headerFont, textPaint);
    y += 40;
    
    // Plugin count
    int pluginCount = host_.getKnownPlugins().getNumTypes();
    textPaint.setColor(SkColorSetARGB(180, 255, 255, 255));
    canvas->drawString("Known Plugins:", 20, y, labelFont, textPaint);
    textPaint.setColor(SK_ColorWHITE);
    canvas->drawString(std::to_string(pluginCount).c_str(), 140, y, valueFont, textPaint);
    y += 30;
    
    // Scanning status
    if (host_.isScanningPlugins()) {
      // Show scanning status
      textPaint.setColor(SkColorSetARGB(255, 100, 255, 150)); // Green tint
      canvas->drawString("Scanning...", 20, y, labelFont, textPaint);
      y += 25;
      
      // Show current plugin
      auto currentPlugin = host_.getCurrentlyScanning();
      if (currentPlugin.isNotEmpty()) {
        textPaint.setColor(SkColorSetARGB(200, 255, 255, 255));
        canvas->drawString(currentPlugin.toStdString().c_str(), 30, y, smallFont, textPaint);
        y += 20;
      }
      
      // Draw progress bar
      float barX = 20;
      float barY = y;
      float barW = getWidth() - 40.0f;
      float barH = 8;
      
      // Background
      SkPaint barBgPaint;
      barBgPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
      barBgPaint.setAntiAlias(true);
      canvas->drawRoundRect(SkRect::MakeXYWH(barX, barY, barW, barH), 4, 4, barBgPaint);
      
      // Progress (pulsing animation for indeterminate)
      SkPaint barFgPaint;
      barFgPaint.setColor(SkColorSetARGB(255, 100, 200, 255)); // Cyan
      barFgPaint.setAntiAlias(true);
      
      // Animate progress bar
      float progress = (std::sin(juce::Time::getMillisecondCounterHiRes() / 200.0f) + 1.0f) / 2.0f;
      float progressW = barW * 0.3f;
      float progressX = barX + progress * (barW - progressW);
      canvas->drawRoundRect(SkRect::MakeXYWH(progressX, barY, progressW, barH), 4, 4, barFgPaint);
      
      y += 30;
    }
    else {
      y += 10;
    }
    
    // Search Paths section
    textPaint.setColor(SkColorSetARGB(180, 255, 255, 255));
    canvas->drawString("Search Paths:", 20, y, labelFont, textPaint);
    y += 20;
    
    auto paths = host_.getSearchPaths();
    textPaint.setColor(SkColorSetARGB(150, 255, 255, 255));
    if (paths.size() == 0) {
      canvas->drawString("(Default paths only)", 30, y, smallFont, textPaint);
      y += 18;
    } else {
      for (const auto& path : paths) {
        canvas->drawString(path.toStdString().c_str(), 30, y, smallFont, textPaint);
        y += 18;
        if (y > getHeight() - 150) break; // Prevent overflow
      }
    }
    
    y += 15;
    
    // Blacklist section
    auto blacklist = host_.getBlacklistedPlugins();
    if (blacklist.size() > 0) {
      textPaint.setColor(SkColorSetARGB(255, 255, 100, 100)); // Red tint
      canvas->drawString(("Blacklisted Plugins: " + std::to_string(blacklist.size())).c_str(), 
                         20, y, labelFont, textPaint);
      y += 20;
      
      textPaint.setColor(SkColorSetARGB(150, 255, 150, 150));
      for (int i = 0; i < std::min((int)blacklist.size(), 5); ++i) {
        juce::File f(blacklist[i]);
        canvas->drawString(f.getFileName().toStdString().c_str(), 30, y, smallFont, textPaint);
        y += 18;
      }
      if (blacklist.size() > 5) {
        canvas->drawString(("... and " + std::to_string(blacklist.size() - 5) + " more").c_str(), 
                           30, y, smallFont, textPaint);
      }
    }
  }

private:
  void startScan() {
    if (host_.isScanningPlugins()) return;
    
    scanButton_->setText("Scanning...");
    cancelButton_->setVisible(true);
    markDirty();
    
    host_.scanAsync([this](int progress, int count, const juce::String &msg) {
      // This callback runs on message thread
      if (progress >= 100) {
        scanButton_->setText("Scan Plugins");
        cancelButton_->setVisible(false);
      }
      markDirty();
    });
  }
  
  void addCustomPath() {
    juce::FileChooser chooser("Select Plugin Folder",
                              juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                              "", true);
                              
    chooser.launchAsync(juce::FileBrowserComponent::openMode | 
                        juce::FileBrowserComponent::canSelectDirectories,
                        [this](const juce::FileChooser& fc) {
      auto result = fc.getResult();
      if (result.exists()) {
        host_.addSearchPath(result.getFullPathName());
        markDirty();
      }
    });
  }

  PluginHost &host_;
  std::unique_ptr<SkiaButton> scanButton_;
  std::unique_ptr<SkiaButton> cancelButton_;
  std::unique_ptr<SkiaButton> addPathButton_;
  std::unique_ptr<SkiaButton> clearBlacklistButton_;
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

    // Create Sidebar Buttons
    createNavButton("Audio", 0);
    createNavButton("Display", 1);
    createNavButton("Plugins", 2);

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

  SkiaComponent *currentTab_ = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)
};

} // namespace zenith