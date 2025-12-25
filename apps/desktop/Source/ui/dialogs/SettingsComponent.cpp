/*
  ==============================================================================

    SettingsComponent.cpp
    Created: 2025-12-25
    Author:  Zenith DAW Team

    Implementation of the flagship Settings Panel.
    Extracted from SettingsComponent.h for faster compilation.

  ==============================================================================
*/

#include "SettingsComponent.h"
#include "../design-system/ColorBridge.h"

namespace zenith {

//==============================================================================
// SettingsTab Base
//==============================================================================

void SettingsTab::drawSkia(SkCanvas* canvas) {
    canvas->clear(SK_ColorTRANSPARENT);
}

//==============================================================================
// AudioSettingsTab Implementation
//==============================================================================

AudioSettingsTab::AudioSettingsTab(Engine& engine) : engine_(engine) {
    setupButton_ = std::make_unique<SkiaButton>("Configure Audio Device...");
    setupButton_->setStyle(SkiaButton::Style::Primary);
    setupButton_->onClick = [this]() { showDeviceSelector(); };
    addAndMakeVisible(setupButton_.get());

    startTimer(500);

    backendSelector_ = std::make_unique<SkiaComboBox>("Linux Audio Backend");
    backendSelector_->addItem("Auto (Recommended)", 1);
    backendSelector_->addItem("JACK", 2);
    backendSelector_->addItem("PipeWire (JACK Bridge)", 3);
    backendSelector_->addItem("ALSA", 4);

    auto currentBackend = Settings::getInstance().getLinuxAudioBackend();
    backendSelector_->setSelectedId((int)currentBackend + 1, false);

    backendSelector_->onChange = [this]() {
        auto selId = backendSelector_->getSelectedId();
        Settings::getInstance().setLinuxAudioBackend(
            (Settings::LinuxAudioBackend)(selId - 1));
    };
    addAndMakeVisible(backendSelector_.get());

    backendLabel_ = std::make_unique<SkiaLabel>("Advanced: Linux Audio Priority");
    addAndMakeVisible(backendLabel_.get());
}

void AudioSettingsTab::resized() {
    if (setupButton_) setupButton_->setBounds(20, 180, 200, 36);
    if (backendSelector_) backendSelector_->setBounds(20, 260, 250, 30);
    if (backendLabel_) backendLabel_->setBounds(20, 235, 300, 20);
}

void AudioSettingsTab::timerCallback() { markDirty(); }

void AudioSettingsTab::drawSkia(SkCanvas* canvas) {
    auto* device = engine_.getDeviceManager().getCurrentAudioDevice();

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

    canvas->drawString("Audio Settings", 20, 40, headerFont, textPaint);

    if (device) {
        float y = 80;
        float labelX = 20;
        float valueX = 140;
        float rowH = 30;

        auto drawRow = [&](const char* label, juce::String value) {
            textPaint.setColor(design::unified::text_secondary());
            canvas->drawString(label, labelX, y, labelFont, textPaint);

            textPaint.setColor(design::unified::text_primary());
            canvas->drawString(value.toStdString().c_str(), valueX, y, valueFont, textPaint);
            y += rowH;
        };

        drawRow("Device Type:", device->getTypeName());
        drawRow("Device Name:", device->getName());
        drawRow("Sample Rate:", juce::String(device->getCurrentSampleRate()) + " Hz");
        drawRow("Buffer Size:", juce::String(device->getCurrentBufferSizeSamples()) + " samples");
    } else {
        textPaint.setColor(design::unified::error());
        canvas->drawString("No Audio Device Selected", 20, 80, valueFont, textPaint);
    }
}

void AudioSettingsTab::showDeviceSelector() {
    juce::DialogWindow::LaunchOptions options;
    auto* content = new juce::AudioDeviceSelectorComponent(
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

//==============================================================================
// DisplaySettingsTab Implementation
//==============================================================================

DisplaySettingsTab::DisplaySettingsTab() {
    fpsSlider_ = std::make_unique<SkiaSlider>("Target FPS");
    fpsSlider_->setStyle(SkiaSlider::Style::Bar);
    fpsSlider_->setDisplayRange(30, 240);
    fpsSlider_->setValue((float)Settings::getInstance().getTargetFPS());
    fpsSlider_->onValueChange = [](float v) {
        Settings::getInstance().setTargetFPS((int)v);
    };
    addAndMakeVisible(fpsSlider_.get());

    glowSlider_ = std::make_unique<SkiaSlider>("Glow Intensity");
    glowSlider_->setStyle(SkiaSlider::Style::Bar);
    glowSlider_->setDisplayRange(0.0f, 2.0f);
    glowSlider_->setValue(Settings::getInstance().getGlowIntensity());
    glowSlider_->onValueChange = [](float v) {
        Settings::getInstance().setGlowIntensity(v);
    };
    addAndMakeVisible(glowSlider_.get());
}

void DisplaySettingsTab::resized() {
    fpsSlider_->setBounds(20, 80, 300, 30);
    glowSlider_->setBounds(20, 150, 300, 30);
}

void DisplaySettingsTab::drawSkia(SkCanvas* canvas) {
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

//==============================================================================
// PluginSettingsTab Implementation
//==============================================================================

PluginSettingsTab::PluginSettingsTab(PluginHost& host) : host_(host) {
    scanButton_ = std::make_unique<SkiaButton>("Scan Plugins");
    scanButton_->setStyle(SkiaButton::Style::Success);
    scanButton_->onClick = [this]() { startScan(); };
    addAndMakeVisible(scanButton_.get());

    pathList_.setMultiLine(true);
    pathList_.setReadOnly(true);
    pathList_.setColour(juce::TextEditor::backgroundColourId, ZenithTheme::Colors::bg_00);
    pathList_.setColour(juce::TextEditor::outlineColourId, ZenithTheme::Colors::border_default);
    addAndMakeVisible(pathList_);

    updateList();
}

void PluginSettingsTab::resized() {
    pathList_.setBounds(20, 80, getWidth() - 40, getHeight() - 140);
    scanButton_->setBounds(getWidth() - 140, getHeight() - 50, 120, 36);
}

void PluginSettingsTab::drawSkia(SkCanvas* canvas) {
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

void PluginSettingsTab::startScan() {
    scanButton_->setText("Scanning...");
    host_.scanAsync([this](int p, int c, const juce::String& m) {
        if (p >= 100) scanButton_->setText("Scan Plugins");
    });
}

void PluginSettingsTab::updateList() {
    juce::String text;
    for (const auto& path : host_.getSearchPaths()) {
        text += path + "\n";
    }
    pathList_.setText(text);
}

//==============================================================================
// AISettingsTab Implementation
//==============================================================================

AISettingsTab::AISettingsTab() {
    apiKeyEditor_ = std::make_unique<juce::TextEditor>("API Key");
    apiKeyEditor_->setMultiLine(false);
    apiKeyEditor_->setPasswordCharacter('*');
    apiKeyEditor_->setTextToShowWhenEmpty("Enter xAI API Key (xai-...)",
                                          juce::Colours::white.withAlpha(0.5f));
    apiKeyEditor_->setColour(juce::TextEditor::backgroundColourId,
                             juce::Colours::transparentBlack);
    apiKeyEditor_->setColour(juce::TextEditor::outlineColourId,
                             juce::Colours::white.withAlpha(0.2f));
    apiKeyEditor_->setColour(juce::TextEditor::textColourId, juce::Colours::white);

    juce::String existingKey;
    if (SecureKeyStore::retrieveKey(SecureKeyStore::GrokAPIKey, existingKey)) {
        apiKeyEditor_->setText(existingKey);
    }
    addAndMakeVisible(apiKeyEditor_.get());

    validateButton_ = std::make_unique<SkiaButton>("Verify & Save Key");
    validateButton_->setStyle(SkiaButton::Style::Primary);
    validateButton_->onClick = [this]() { validateKey(); };
    addAndMakeVisible(validateButton_.get());

    helpLabel_ = std::make_unique<SkiaLabel>("Get an API key at console.x.ai");
    addAndMakeVisible(helpLabel_.get());

    statusLabel_ = std::make_unique<SkiaLabel>("");
    addAndMakeVisible(statusLabel_.get());
}

void AISettingsTab::resized() {
    apiKeyEditor_->setBounds(20, 80, 400, 30);
    validateButton_->setBounds(430, 80, 150, 30);
    helpLabel_->setBounds(20, 115, 300, 20);
    statusLabel_->setBounds(20, 140, 400, 20);
}

void AISettingsTab::drawSkia(SkCanvas* canvas) {
    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);

    SkFont headerFont;
    headerFont.setSize(24.0f);
    headerFont.setEmbolden(true);

    SkFont labelFont;
    labelFont.setSize(14.0f);

    canvas->drawString("AI Integration", 20, 40, headerFont, textPaint);
    canvas->drawString("Grok API Configuration", 20, 70, labelFont, textPaint);
}

void AISettingsTab::validateKey() {
    juce::String key = apiKeyEditor_->getText().trim();
    if (key.isEmpty()) {
        statusLabel_->setText("Please enter an API key");
        return;
    }

    validateButton_->setText("Verifying...");
    validateButton_->setEnabled(false);

    testClient_ = std::make_unique<GrokAPIClient>();
    testClient_->setAPIKey(key);

    testClient_->sendChat(
        "Release check", GrokMode::Fast, {}, "",
        [this, key](juce::String response) {
            juce::MessageManager::callAsync([this, key]() {
                SecureKeyStore::storeKey(SecureKeyStore::GrokAPIKey, key);
                statusLabel_->setText("Success! Key verified and saved.");
                validateButton_->setText("Verify & Save Key");
                validateButton_->setEnabled(true);
                testClient_ = nullptr;
            });
        },
        nullptr,
        [this](juce::String error) {
            juce::MessageManager::callAsync([this, error]() {
                statusLabel_->setText("Error: " + error);
                validateButton_->setText("Verify & Save Key");
                validateButton_->setEnabled(true);
                testClient_ = nullptr;
            });
        });
}

//==============================================================================
// SettingsComponent Implementation
//==============================================================================

SettingsComponent::SettingsComponent(Engine& engine) : engine_(engine) {
    setSize(800, 600);

    audioTab_ = std::make_unique<AudioSettingsTab>(engine);
    addChildComponent(audioTab_.get());

    displayTab_ = std::make_unique<DisplaySettingsTab>();
    addChildComponent(displayTab_.get());

    pluginTab_ = std::make_unique<PluginSettingsTab>(engine.getPluginHost());
    addChildComponent(pluginTab_.get());

    aiTab_ = std::make_unique<AISettingsTab>();
    addChildComponent(aiTab_.get());

    hardwareTab_ = std::make_unique<HardwareControlPanel>(engine);
    addChildComponent(hardwareTab_.get());

    createNavButton("Audio", 0);
    createNavButton("Display", 1);
    createNavButton("Plugins", 2);
    createNavButton("AI", 3);
    createNavButton("Hardware", 4);

    setActiveTab(0);
}

SettingsComponent::~SettingsComponent() = default;

void SettingsComponent::resized() {
    int sidebarWidth = 200;
    int btnHeight = 40;
    int y = 20;

    for (auto* btn : navButtons_) {
        btn->setBounds(10, y, sidebarWidth - 20, btnHeight);
        y += btnHeight + 5;
    }

    auto contentArea = getLocalBounds().removeFromRight(getWidth() - sidebarWidth);
    if (currentTab_) currentTab_->setBounds(contentArea);
}

void SettingsComponent::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds();
    int sidebarWidth = 200;

    canvas->clear(design::unified::bg_00());

    SkPaint sidebarPaint;
    sidebarPaint.setColor(design::unified::bg_01());
    canvas->drawRect(SkRect::MakeWH((float)sidebarWidth, (float)bounds.getHeight()), sidebarPaint);

    SkPaint linePaint;
    linePaint.setColor(design::unified::border_subtle());
    canvas->drawLine((float)sidebarWidth, 0, (float)sidebarWidth, (float)bounds.getHeight(), linePaint);
}

void SettingsComponent::createNavButton(const juce::String& name, int index) {
    auto btn = std::make_unique<SkiaButton>(name);
    btn->setStyle(SkiaButton::Style::Ghost);
    btn->setToggleable(true);
    btn->onClick = [this, index]() { setActiveTab(index); };
    addAndMakeVisible(btn.get());
    navButtons_.add(btn.release());
}

void SettingsComponent::setActiveTab(int index) {
    for (int i = 0; i < navButtons_.size(); ++i) {
        navButtons_[i]->setToggleState(i == index);
        navButtons_[i]->setStyle(i == index ? SkiaButton::Style::Secondary : SkiaButton::Style::Ghost);
    }

    if (currentTab_) currentTab_->setVisible(false);

    switch (index) {
        case 0: currentTab_ = audioTab_.get(); break;
        case 1: currentTab_ = displayTab_.get(); break;
        case 2: currentTab_ = pluginTab_.get(); break;
        case 3: currentTab_ = aiTab_.get(); break;
        case 4: currentTab_ = hardwareTab_.get(); break;
    }

    if (currentTab_) {
        currentTab_->setVisible(true);
        resized();
    }
}

} // namespace zenith
