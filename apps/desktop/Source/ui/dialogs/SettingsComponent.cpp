/*
  ==============================================================================

    SettingsComponent.cpp
    Created: 2025-12-07
    Author:  Zenith DAW Team

  ==============================================================================
*/

#include "SettingsComponent.h"
#include "Settings.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>

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
    // Scan Button
    scanButton_ = std::make_unique<SkiaButton>("Scan Plugins");
    scanButton_->setStyle(SkiaButton::Style::Success);
    scanButton_->onClick = [this]() { startScan(); };
    addAndMakeVisible(scanButton_.get());

    // Search Paths
    pathList_.setMultiLine(true);
    pathList_.setReadOnly(true);
    pathList_.setColour(juce::TextEditor::backgroundColourId,
                        juce::Colours::transparentBlack);
    pathList_.setColour(juce::TextEditor::outlineColourId,
                        juce::Colours::white.withAlpha(0.2f));
    pathList_.setText("Search Paths:\n" +
                      host_.getSearchPaths().joinIntoString("\n"));
    addAndMakeVisible(pathList_);

    // Blacklist
    blacklistList_.setMultiLine(true);
    blacklistList_.setReadOnly(true);
    blacklistList_.setColour(juce::TextEditor::backgroundColourId,
                             juce::Colours::transparentBlack);
    blacklistList_.setColour(
        juce::TextEditor::outlineColourId,
        juce::Colours::red.withAlpha(0.2f)); // Red tint for danger
    addAndMakeVisible(blacklistList_);

    // Clear Blacklist Button
    clearBlacklistButton_ = std::make_unique<SkiaButton>("Clear Blacklist");
    clearBlacklistButton_->setStyle(SkiaButton::Style::Danger);
    clearBlacklistButton_->onClick = [this]() {
      host_.clearBlacklist();
      updateBlacklist();
    };
    addAndMakeVisible(clearBlacklistButton_.get());

    updateBlacklist();
  }

  void resized() override {
    auto bounds = getLocalBounds().reduced(20);
    int buttonHeight = 36;
    int spacing = 10;

    // Layout:
    // Header (drawn in drawSkia)
    // Scan Button (top right)
    // Path List (top half)
    // Blacklist Header (drawn in drawSkia) - implicit
    // Blacklist List (bottom half)
    // Clear Blacklist Button (bottom right)

    scanButton_->setBounds(bounds.getRight() - 120, bounds.getY() + 40, 120,
                           buttonHeight);

    int listHeight = (bounds.getHeight() - 150) / 2;
    pathList_.setBounds(bounds.getX(), bounds.getY() + 80, bounds.getWidth(),
                        listHeight);

    int blacklistGroupY = pathList_.getBottom() + 40;
    blacklistList_.setBounds(bounds.getX(), blacklistGroupY, bounds.getWidth(),
                             listHeight);

    clearBlacklistButton_->setBounds(bounds.getRight() - 140,
                                     blacklistGroupY - 35, 140, 30);
  }

  void drawSkia(SkCanvas *canvas) override {
    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);

    SkFont headerFont;
    headerFont.setSize(24.0f);
    headerFont.setEmbolden(true);

    SkFont subHeaderFont;
    subHeaderFont.setSize(18.0f);
    subHeaderFont.setEmbolden(true);

    // Main Header
    canvas->drawString("Plugin Management", 20, 40, headerFont, textPaint);

    // Subheaders rely on bounds calc, roughly:
    canvas->drawString("Search Paths", 20, 70, subHeaderFont, textPaint);

    float blacklistY = (float)pathList_.getBottom() + 30;
    textPaint.setColor(SkColorSetRGB(255, 100, 100)); // Red ish
    canvas->drawString("Blacklisted Plugins (Crashed/Timed Out)", 20,
                       blacklistY, subHeaderFont, textPaint);

    if (scanning_) {
      SkPaint statusPaint;
      statusPaint.setColor(SkColorSetRGB(100, 255, 100)); // Green
      SkFont statusFont;
      statusFont.setSize(14.0f);
      canvas->drawString(scanStatus_.toStdString().c_str(), 200, 55, statusFont,
                         statusPaint);
    }
  }

private:
  void startScan() {
    if (scanning_)
      return;
    scanning_ = true;
    scanButton_->setText("Scanning...");

    host_.scanAsync([this](int progress, int total, const juce::String &msg) {
      if (progress == 100) {
        scanning_ = false;
        scanButton_->setText("Scan Plugins");
        scanStatus_ = "Scan Complete.";
        updateBlacklist();
        repaint();
      } else {
        scanStatus_ = "Scanning: " + msg;
        repaint();
      }
    });
  }

  void updateBlacklist() {
    auto blacklist = host_.getBlacklistedPlugins();
    if (blacklist.isEmpty()) {
      blacklistList_.setText("No plugins blacklisted.");
    } else {
      blacklistList_.setText(blacklist.joinIntoString("\n"));
    }
    repaint();
  }

  PluginHost &host_;
  std::unique_ptr<SkiaButton> scanButton_;
  std::unique_ptr<SkiaButton> clearBlacklistButton_;
  juce::TextEditor pathList_;
  juce::TextEditor blacklistList_;

  bool scanning_ = false;
  juce::String scanStatus_;
};

//==============================================================================
// Settings Component
//==============================================================================

SettingsComponent::SettingsComponent(Engine &engine) : engine_(engine) {
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

SettingsComponent::~SettingsComponent() = default;

void SettingsComponent::resized() {
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

void SettingsComponent::drawSkia(SkCanvas *canvas) {
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

void SettingsComponent::createNavButton(const juce::String &name, int index) {
  auto btn = std::make_unique<SkiaButton>(name);
  btn->setStyle(SkiaButton::Style::Ghost);
  btn->setToggleable(true);
  btn->onClick = [this, index]() { setActiveTab(index); };
  addAndMakeVisible(btn.get());
  navButtons_.add(btn.release());
}

void SettingsComponent::setActiveTab(int index) {
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

} // namespace zenith
