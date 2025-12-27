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

    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimer(500);

    backendSelector_ = std::make_unique<SkiaComboBox>("Linux Audio Backend");
    backendSelector_->addItem("Auto (Recommended)", 1);
    backendSelector_->addItem("JACK", 2);
    backendSelector_->addItem("PipeWire (JACK Bridge)", 3);
    backendSelector_->addItem("ALSA", 4);
    auto currentBackend = Settings::getInstance().getLinuxAudioBackend();
    backendSelector_->setSelectedId((int)currentBackend + 1, false);
    backendSelector_->onChange = [this]() {
        auto selId = backendSelector_->getSelectedId();
        Settings::getInstance().setLinuxAudioBackend((Settings::LinuxAudioBackend)(selId - 1));
    };
    addAndMakeVisible(backendSelector_.get());

    backendLabel_ = std::make_unique<SkiaLabel>("Advanced: Linux Audio Priority");
    addAndMakeVisible(backendLabel_.get());

    // Buffer size selector
    bufferSizeSelector_ = std::make_unique<SkiaComboBox>("Buffer Size");
    bufferSizeSelector_->addItem("32 samples", 32);
    bufferSizeSelector_->addItem("64 samples", 64);
    bufferSizeSelector_->addItem("128 samples", 128);
    bufferSizeSelector_->addItem("256 samples", 256);
    bufferSizeSelector_->addItem("512 samples", 512);
    bufferSizeSelector_->addItem("1024 samples", 1024);
    bufferSizeSelector_->addItem("2048 samples", 2048);
    bufferSizeSelector_->addItem("4096 samples", 4096);
    bufferSizeSelector_->setSelectedId(Settings::getInstance().getBufferSize(), false);
    bufferSizeSelector_->onChange = [this]() {
        Settings::getInstance().setBufferSize(bufferSizeSelector_->getSelectedId());
    };
    addAndMakeVisible(bufferSizeSelector_.get());

    bufferLabel_ = std::make_unique<SkiaLabel>("Preferred Buffer Size");
    addAndMakeVisible(bufferLabel_.get());

    // Plugin Delay Compensation toggle
    pdcToggle_ = std::make_unique<SkiaButton>("Plugin Delay Compensation");
    pdcToggle_->setStyle(SkiaButton::Style::Secondary);
    pdcToggle_->setToggleable(true);
    pdcToggle_->setToggleState(Settings::getInstance().getPluginDelayCompensation());
    pdcToggle_->onClick = [this]() {
        Settings::getInstance().setPluginDelayCompensation(pdcToggle_->getToggleState());
    };
    addAndMakeVisible(pdcToggle_.get());

    // Software Monitoring toggle
    monitoringToggle_ = std::make_unique<SkiaButton>("Software Monitoring");
    monitoringToggle_->setStyle(SkiaButton::Style::Secondary);
    monitoringToggle_->setToggleable(true);
    monitoringToggle_->setToggleState(Settings::getInstance().getSoftwareMonitoring());
    monitoringToggle_->onClick = [this]() {
        Settings::getInstance().setSoftwareMonitoring(monitoringToggle_->getToggleState());
    };
    addAndMakeVisible(monitoringToggle_.get());

    // Monitoring volume slider
    monitoringVolumeSlider_ = std::make_unique<SkiaSlider>("Monitoring Volume", design::colors::CYAN);
    monitoringVolumeSlider_->setRange(0.0f, 1.0f, Settings::getInstance().getMonitoringVolume());
    monitoringVolumeSlider_->setValue(Settings::getInstance().getMonitoringVolume());
    monitoringVolumeSlider_->onValueChange = [](float v) {
        Settings::getInstance().setMonitoringVolume(v);
    };
    addAndMakeVisible(monitoringVolumeSlider_.get());
}

void AudioSettingsTab::resized() {
    int y = 180;
    if (setupButton_) setupButton_->setBounds(20, y, 200, 36); y += 50;
    if (bufferLabel_) bufferLabel_->setBounds(20, y, 200, 20); y += 25;
    if (bufferSizeSelector_) bufferSizeSelector_->setBounds(20, y, 200, 30); y += 45;
    if (backendLabel_) backendLabel_->setBounds(20, y, 300, 20); y += 25;
    if (backendSelector_) backendSelector_->setBounds(20, y, 250, 30); y += 50;
    if (pdcToggle_) pdcToggle_->setBounds(20, y, 250, 30); y += 40;
    if (monitoringToggle_) monitoringToggle_->setBounds(20, y, 200, 30); y += 40;
    if (monitoringVolumeSlider_) monitoringVolumeSlider_->setBounds(20, y, 250, 30);
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
    fpsSlider_ = std::make_unique<SkiaSlider>("Target FPS", design::colors::CYAN);
    fpsSlider_->setRange(30, 240, (float)Settings::getInstance().getTargetFPS());
    fpsSlider_->setValue((float)Settings::getInstance().getTargetFPS());
    fpsSlider_->onValueChange = [](float v) {
        Settings::getInstance().setTargetFPS((int)v);
    };
    addAndMakeVisible(fpsSlider_.get());

    glowSlider_ = std::make_unique<SkiaSlider>("Glow Intensity", design::colors::CYAN);
    glowSlider_->setRange(0.0f, 2.0f, Settings::getInstance().getGlowIntensity());
    glowSlider_->setValue(Settings::getInstance().getGlowIntensity());
    glowSlider_->onValueChange = [](float v) {
        Settings::getInstance().setGlowIntensity(v);
    };
    addAndMakeVisible(glowSlider_.get());

    themeSelector_ = std::make_unique<SkiaComboBox>("UI Theme");
    themeSelector_->addItem("Dark (Default)", 1);
    themeSelector_->addItem("Darker (OLED)", 2);
    themeSelector_->addItem("Light (Classic)", 3);
    
    auto currentPreset = design::ThemeManager::getInstance().getActiveTheme();
    themeSelector_->setSelectedId((int)currentPreset + 1, false);
    
    themeSelector_->onChange = [this]() {
        auto preset = static_cast<design::ThemePreset>(themeSelector_->getSelectedId() - 1);
        design::ThemeManager::getInstance().setActiveTheme(preset);
    };
    addAndMakeVisible(themeSelector_.get());
}

void DisplaySettingsTab::resized() {
    fpsSlider_->setBounds(20, 80, 300, 30);
    glowSlider_->setBounds(20, 150, 300, 30);
    themeSelector_->setBounds(20, 220, 300, 30);
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
    canvas->drawString("UI Theme Preset", 20, 210, labelFont, textPaint);
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

    testClient_ = std::make_unique<GrokDAWClient>();
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
// RecordingSettingsTab Implementation
//==============================================================================

RecordingSettingsTab::RecordingSettingsTab() {
    countInSlider_ = std::make_unique<SkiaSlider>("Count-In Bars", design::colors::CYAN);
    countInSlider_->setRange(0, 4, (float)Settings::getInstance().getCountInBars());
    countInSlider_->setValue((float)Settings::getInstance().getCountInBars());
    countInSlider_->onValueChange = [](float v) {
        Settings::getInstance().setCountInBars((int)v);
    };
    addAndMakeVisible(countInSlider_.get());

    metronomeCountInToggle_ = std::make_unique<SkiaButton>("Metronome During Count-In");
    metronomeCountInToggle_->setStyle(SkiaButton::Style::Secondary);
    metronomeCountInToggle_->setToggleable(true);
    metronomeCountInToggle_->setToggleState(Settings::getInstance().getMetronomeCountIn());
    metronomeCountInToggle_->onClick = [this]() {
        Settings::getInstance().setMetronomeCountIn(metronomeCountInToggle_->getToggleState());
    };
    addAndMakeVisible(metronomeCountInToggle_.get());

    bitDepthSelector_ = std::make_unique<SkiaComboBox>("Bit Depth");
    bitDepthSelector_->addItem("16-bit", 1);
    bitDepthSelector_->addItem("24-bit", 2);
    bitDepthSelector_->addItem("32-bit Float", 3);
    bitDepthSelector_->setSelectedId((int)Settings::getInstance().getRecordingBitDepth() + 1, false);
    bitDepthSelector_->onChange = [this]() {
        Settings::getInstance().setRecordingBitDepth((Settings::RecordingBitDepth)(bitDepthSelector_->getSelectedId() - 1));
    };
    addAndMakeVisible(bitDepthSelector_.get());

    fileTypeSelector_ = std::make_unique<SkiaComboBox>("File Format");
    fileTypeSelector_->addItem("WAV", 1);
    fileTypeSelector_->addItem("AIFF", 2);
    fileTypeSelector_->addItem("FLAC", 3);
    fileTypeSelector_->setSelectedId((int)Settings::getInstance().getRecordingFileType() + 1, false);
    fileTypeSelector_->onChange = [this]() {
        Settings::getInstance().setRecordingFileType((Settings::RecordingFileType)(fileTypeSelector_->getSelectedId() - 1));
    };
    addAndMakeVisible(fileTypeSelector_.get());

    tempoLockToggle_ = std::make_unique<SkiaButton>("Lock Tempo During Recording");
    tempoLockToggle_->setStyle(SkiaButton::Style::Secondary);
    tempoLockToggle_->setToggleable(true);
    tempoLockToggle_->setToggleState(!Settings::getInstance().getAllowTempoChangeDuringRecord());
    tempoLockToggle_->onClick = [this]() {
        Settings::getInstance().setAllowTempoChangeDuringRecord(!tempoLockToggle_->getToggleState());
    };
    addAndMakeVisible(tempoLockToggle_.get());
}

void RecordingSettingsTab::resized() {
    int y = 80;
    if (countInSlider_) { countInSlider_->setBounds(20, y, 250, 30); y += 50; }
    if (metronomeCountInToggle_) { metronomeCountInToggle_->setBounds(20, y, 250, 30); y += 50; }
    if (bitDepthSelector_) { bitDepthSelector_->setBounds(20, y, 180, 30); y += 50; }
    if (fileTypeSelector_) { fileTypeSelector_->setBounds(20, y, 180, 30); y += 50; }
    if (tempoLockToggle_) { tempoLockToggle_->setBounds(20, y, 260, 30); }
}

void RecordingSettingsTab::drawSkia(SkCanvas* canvas) {
    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);
    SkFont headerFont;
    headerFont.setSize(24.0f);
    headerFont.setEmbolden(true);
    canvas->drawString("Recording Settings", 20, 40, headerFont, textPaint);
}

//==============================================================================
// MIDISettingsTab Implementation
//==============================================================================

MIDISettingsTab::MIDISettingsTab() {
    midiThroughToggle_ = std::make_unique<SkiaButton>("MIDI Thru");
    midiThroughToggle_->setStyle(SkiaButton::Style::Secondary);
    midiThroughToggle_->setToggleable(true);
    midiThroughToggle_->setToggleState(Settings::getInstance().getMIDIThrough());
    midiThroughToggle_->onClick = [this]() {
        Settings::getInstance().setMIDIThrough(midiThroughToggle_->getToggleState());
    };
    addAndMakeVisible(midiThroughToggle_.get());

    midiClockOutToggle_ = std::make_unique<SkiaButton>("Send MIDI Clock");
    midiClockOutToggle_->setStyle(SkiaButton::Style::Secondary);
    midiClockOutToggle_->setToggleable(true);
    midiClockOutToggle_->setToggleState(Settings::getInstance().getSendMIDIClockOut());
    midiClockOutToggle_->onClick = [this]() {
        Settings::getInstance().setSendMIDIClockOut(midiClockOutToggle_->getToggleState());
    };
    addAndMakeVisible(midiClockOutToggle_.get());

    mtcInToggle_ = std::make_unique<SkiaButton>("Receive MTC");
    mtcInToggle_->setStyle(SkiaButton::Style::Secondary);
    mtcInToggle_->setToggleable(true);
    mtcInToggle_->setToggleState(Settings::getInstance().getReceiveMTCIn());
    mtcInToggle_->onClick = [this]() {
        Settings::getInstance().setReceiveMTCIn(mtcInToggle_->getToggleState());
    };
    addAndMakeVisible(mtcInToggle_.get());

    latencyCompSlider_ = std::make_unique<SkiaSlider>("MIDI Latency Offset (ms)", design::colors::CYAN);
    latencyCompSlider_->setRange(-50, 50, (float)Settings::getInstance().getMIDILatencyCompensation());
    latencyCompSlider_->setValue((float)Settings::getInstance().getMIDILatencyCompensation());
    latencyCompSlider_->onValueChange = [](float v) {
        Settings::getInstance().setMIDILatencyCompensation((int)v);
    };
    addAndMakeVisible(latencyCompSlider_.get());
}

void MIDISettingsTab::resized() {
    int y = 80;
    if (midiThroughToggle_) { midiThroughToggle_->setBounds(20, y, 200, 30); y += 50; }
    if (midiClockOutToggle_) { midiClockOutToggle_->setBounds(20, y, 200, 30); y += 50; }
    if (mtcInToggle_) { mtcInToggle_->setBounds(20, y, 200, 30); y += 50; }
    if (latencyCompSlider_) { latencyCompSlider_->setBounds(20, y, 280, 30); }
}

void MIDISettingsTab::drawSkia(SkCanvas* canvas) {
    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);
    SkFont headerFont;
    headerFont.setSize(24.0f);
    headerFont.setEmbolden(true);
    canvas->drawString("MIDI Settings", 20, 40, headerFont, textPaint);
}

//==============================================================================
// EditingSettingsTab Implementation
//==============================================================================

EditingSettingsTab::EditingSettingsTab() {
    crossfadeSlider_ = std::make_unique<SkiaSlider>("Default Crossfade (ms)", design::colors::CYAN);
    crossfadeSlider_->setRange(0, 100, (float)Settings::getInstance().getDefaultCrossfadeMs());
    crossfadeSlider_->setValue((float)Settings::getInstance().getDefaultCrossfadeMs());
    crossfadeSlider_->onValueChange = [](float v) {
        Settings::getInstance().setDefaultCrossfadeMs((int)v);
    };
    addAndMakeVisible(crossfadeSlider_.get());

    snapToggle_ = std::make_unique<SkiaButton>("Snap to Grid");
    snapToggle_->setStyle(SkiaButton::Style::Secondary);
    snapToggle_->setToggleable(true);
    snapToggle_->setToggleState(Settings::getInstance().getSnapToGrid());
    snapToggle_->onClick = [this]() {
        Settings::getInstance().setSnapToGrid(snapToggle_->getToggleState());
    };
    addAndMakeVisible(snapToggle_.get());

    linkSelectionToggle_ = std::make_unique<SkiaButton>("Link Track & Edit Selection");
    linkSelectionToggle_->setStyle(SkiaButton::Style::Secondary);
    linkSelectionToggle_->setToggleable(true);
    linkSelectionToggle_->setToggleState(Settings::getInstance().getLinkTrackAndEditSelection());
    linkSelectionToggle_->onClick = [this]() {
        Settings::getInstance().setLinkTrackAndEditSelection(linkSelectionToggle_->getToggleState());
    };
    addAndMakeVisible(linkSelectionToggle_.get());
}

void EditingSettingsTab::resized() {
    int y = 80;
    if (crossfadeSlider_) { crossfadeSlider_->setBounds(20, y, 280, 30); y += 50; }
    if (snapToggle_) { snapToggle_->setBounds(20, y, 180, 30); y += 50; }
    if (linkSelectionToggle_) { linkSelectionToggle_->setBounds(20, y, 260, 30); }
}

void EditingSettingsTab::drawSkia(SkCanvas* canvas) {
    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);
    SkFont headerFont;
    headerFont.setSize(24.0f);
    headerFont.setEmbolden(true);
    canvas->drawString("Editing Preferences", 20, 40, headerFont, textPaint);
}

//==============================================================================
// ProjectSettingsTab Implementation
//==============================================================================

ProjectSettingsTab::ProjectSettingsTab() {
    currentProjectFolder_ = Settings::getInstance().getDefaultProjectFolder();

    autoSaveToggle_ = std::make_unique<SkiaButton>("Enable Auto-Save");
    autoSaveToggle_->setStyle(SkiaButton::Style::Secondary);
    autoSaveToggle_->setToggleable(true);
    autoSaveToggle_->setToggleState(Settings::getInstance().getAutoSaveEnabled());
    autoSaveToggle_->onClick = [this]() {
        Settings::getInstance().setAutoSaveEnabled(autoSaveToggle_->getToggleState());
    };
    addAndMakeVisible(autoSaveToggle_.get());

    autoSaveIntervalSlider_ = std::make_unique<SkiaSlider>("Auto-Save Interval (min)", design::colors::CYAN);
    autoSaveIntervalSlider_->setRange(1, 30, (float)Settings::getInstance().getAutoSaveIntervalMinutes());
    autoSaveIntervalSlider_->setValue((float)Settings::getInstance().getAutoSaveIntervalMinutes());
    autoSaveIntervalSlider_->onValueChange = [](float v) {
        Settings::getInstance().setAutoSaveIntervalMinutes((int)v);
    };
    addAndMakeVisible(autoSaveIntervalSlider_.get());

    undoHistorySlider_ = std::make_unique<SkiaSlider>("Max Undo Steps", design::colors::CYAN);
    undoHistorySlider_->setRange(10, 500, (float)Settings::getInstance().getMaxUndoHistory());
    undoHistorySlider_->setValue((float)Settings::getInstance().getMaxUndoHistory());
    undoHistorySlider_->onValueChange = [](float v) {
        Settings::getInstance().setMaxUndoHistory((int)v);
    };
    addAndMakeVisible(undoHistorySlider_.get());

    projectFolderButton_ = std::make_unique<SkiaButton>("Choose Project Folder...");
    projectFolderButton_->setStyle(SkiaButton::Style::Secondary);
    projectFolderButton_->onClick = [this]() {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Select Default Project Folder", 
            juce::File(currentProjectFolder_), "");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | 
                             juce::FileBrowserComponent::canSelectDirectories,
            [this, chooser](const juce::FileChooser& fc) {
                if (fc.getResults().size() > 0) {
                    currentProjectFolder_ = fc.getResult().getFullPathName();
                    Settings::getInstance().setDefaultProjectFolder(currentProjectFolder_);
                    markDirty();
                }
            });
    };
    addAndMakeVisible(projectFolderButton_.get());
}

void ProjectSettingsTab::resized() {
    int y = 80;
    if (autoSaveToggle_) { autoSaveToggle_->setBounds(20, y, 200, 30); y += 50; }
    if (autoSaveIntervalSlider_) { autoSaveIntervalSlider_->setBounds(20, y, 250, 30); y += 50; }
    if (undoHistorySlider_) { undoHistorySlider_->setBounds(20, y, 250, 30); y += 50; }
    if (projectFolderButton_) { projectFolderButton_->setBounds(20, y, 260, 36); }
}

void ProjectSettingsTab::drawSkia(SkCanvas* canvas) {
    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);
    SkFont headerFont;
    headerFont.setSize(24.0f);
    headerFont.setEmbolden(true);
    SkFont labelFont;
    labelFont.setSize(12.0f);
    canvas->drawString("Project Settings", 20, 40, headerFont, textPaint);
    if (!currentProjectFolder_.isEmpty()) {
        textPaint.setColor(design::unified::text_secondary());
        canvas->drawString(("Folder: " + currentProjectFolder_).toStdString().c_str(), 20, 290, labelFont, textPaint);
    }
}

//==============================================================================
// AboutTab Implementation
//==============================================================================

AboutTab::AboutTab() {}

void AboutTab::resized() {}

void AboutTab::drawSkia(SkCanvas* canvas) {
    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);
    
    SkFont headerFont;
    headerFont.setSize(28.0f);
    headerFont.setEmbolden(true);
    
    SkFont versionFont;
    versionFont.setSize(16.0f);
    
    SkFont labelFont;
    labelFont.setSize(14.0f);

    canvas->drawString("Zenith DAW", 20, 50, headerFont, textPaint);
    
    textPaint.setColor(design::unified::accent_primary());
    canvas->drawString("Version 1.0.0-alpha", 20, 80, versionFont, textPaint);
    
    textPaint.setColor(design::unified::text_secondary());
    canvas->drawString("A professional digital audio workstation", 20, 110, labelFont, textPaint);
    canvas->drawString("with Skia-accelerated UI and AI integration.", 20, 130, labelFont, textPaint);
    
    textPaint.setColor(SK_ColorWHITE);
    canvas->drawString("Keyboard Shortcuts:", 20, 180, versionFont, textPaint);
    
    textPaint.setColor(design::unified::text_secondary());
    int y = 210;
    auto drawShortcut = [&](const char* key, const char* action) {
        textPaint.setColor(design::unified::accent_primary());
        canvas->drawString(key, 20, (float)y, labelFont, textPaint);
        textPaint.setColor(design::unified::text_secondary());
        canvas->drawString(action, 120, (float)y, labelFont, textPaint);
        y += 25;
    };
    
    drawShortcut("Space", "Play / Stop");
    drawShortcut("R", "Record");
    drawShortcut("Ctrl+Z", "Undo");
    drawShortcut("Ctrl+Shift+Z", "Redo");
    drawShortcut("Ctrl+S", "Save");
    drawShortcut("Ctrl+,", "Settings");
}

//==============================================================================
// SettingsComponent Implementation
//==============================================================================

SettingsComponent::SettingsComponent(Engine& engine) : engine_(engine) {
    setSize(900, 700);

    audioTab_ = std::make_unique<AudioSettingsTab>(engine);
    addChildComponent(audioTab_.get());

    displayTab_ = std::make_unique<DisplaySettingsTab>();
    addChildComponent(displayTab_.get());

    recordingTab_ = std::make_unique<RecordingSettingsTab>();
    addChildComponent(recordingTab_.get());

    midiTab_ = std::make_unique<MIDISettingsTab>();
    addChildComponent(midiTab_.get());

    editingTab_ = std::make_unique<EditingSettingsTab>();
    addChildComponent(editingTab_.get());

    projectTab_ = std::make_unique<ProjectSettingsTab>();
    addChildComponent(projectTab_.get());

    pluginTab_ = std::make_unique<PluginSettingsTab>(engine.getPluginHost());
    addChildComponent(pluginTab_.get());

    aiTab_ = std::make_unique<AISettingsTab>();
    addChildComponent(aiTab_.get());

    hardwareTab_ = std::make_unique<HardwareControlPanel>(engine);
    addChildComponent(hardwareTab_.get());

    aboutTab_ = std::make_unique<AboutTab>();
    addChildComponent(aboutTab_.get());

    createNavButton("Audio", 0);
    createNavButton("Display", 1);
    createNavButton("Recording", 2);
    createNavButton("MIDI", 3);
    createNavButton("Editing", 4);
    createNavButton("Project", 5);
    createNavButton("Plugins", 6);
    createNavButton("AI", 7);
    createNavButton("Hardware", 8);
    createNavButton("About", 9);

    setActiveTab(0);
}

SettingsComponent::~SettingsComponent() = default;

void SettingsComponent::resized() {
    int sidebarWidth = 180;
    int btnHeight = 32;
    int y = 15;

    for (auto* btn : navButtons_) {
        btn->setBounds(8, y, sidebarWidth - 16, btnHeight);
        y += btnHeight + 4;
    }

    auto contentArea = getLocalBounds().removeFromRight(getWidth() - sidebarWidth);
    if (currentTab_) currentTab_->setBounds(contentArea);
}

void SettingsComponent::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds();
    int sidebarWidth = 180;

    canvas->clear(design::unified::bg_03());

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
        case 2: currentTab_ = recordingTab_.get(); break;
        case 3: currentTab_ = midiTab_.get(); break;
        case 4: currentTab_ = editingTab_.get(); break;
        case 5: currentTab_ = projectTab_.get(); break;
        case 6: currentTab_ = pluginTab_.get(); break;
        case 7: currentTab_ = aiTab_.get(); break;
        case 8: currentTab_ = hardwareTab_.get(); break;
        case 9: currentTab_ = aboutTab_.get(); break;
    }

    if (currentTab_) {
        currentTab_->setVisible(true);
        resized();
    }
}

} // namespace zenith

