/*
  ==============================================================================
    SettingsPanel.cpp
    Settings panel with live preview implementation
  ==============================================================================
*/

#include "SettingsPanel.h"
#include <algorithm>
#include <chrono>

namespace zenith {
namespace ui {

// SettingsPanel Implementation
SettingsPanel::SettingsPanel() {
    // Initialize default settings
    auto defaultSettings = SettingsPanelFactory::getDefaultMasteringSettings();
    auto analysisSettings = SettingsPanelFactory::getDefaultAnalysisSettings();
    auto learningSettings = SettingsPanelFactory::getDefaultLearningSettings();
    auto advancedSettings = SettingsPanelFactory::getDefaultAdvancedSettings();
    
    for (const auto& setting : defaultSettings) {
        registerSetting(setting);
    }
    for (const auto& setting : analysisSettings) {
        registerSetting(setting);
    }
    for (const auto& setting : learningSettings) {
        registerSetting(setting);
    }
    for (const auto& setting : advancedSettings) {
        registerSetting(setting);
    }
    
    // Initialize UI components
    mainViewport = std::make_unique<juce::Viewport>();
    mainComponent = std::make_unique<juce::Component>();
    
    // Create UI
    createCategoryTabs();
    createSearchBar();
    createPresetControls();
    createLivePreviewControls();
    createAIControls();
    createPreviewDisplay();
    createSettingComponents();
    
    // Setup viewport
    mainViewport->setViewedComponent(mainComponent.get(), false);
    mainViewport->setScrollBarsShown(true, false);
    
    addAndMakeVisible(*mainViewport);
    
    // Load presets
    loadPresets();
    
    // Start timer for live updates
    startTimerHz(30);  // 30 FPS
    
    // Initialize preview data
    currentPreview.sampleRate = 44100.0;
}

SettingsPanel::~SettingsPanel() {
    stopTimer();
    savePresets();
}

void SettingsPanel::paint(juce::Graphics& g) {
    // Skia rendering used - no JUCE rendering needed
    juce::ignoreUnused(g);
}

void SettingsPanel::resized() {
    auto bounds = getLocalBounds();
    mainViewport->setBounds(bounds);
    
    // Layout main component
    int margin = 10;
    int topMargin = margin;
    int width = bounds.getWidth() - 2 * margin;
    
    // Search bar at top
    int searchBarHeight = 30;
    if (searchEditor) {
        searchEditor->setBounds(margin, topMargin, width - 100, searchBarHeight);
    }
    if (clearSearchButton) {
        clearSearchButton->setBounds(width - 90, topMargin, 80, searchBarHeight);
    }
    topMargin += searchBarHeight + margin;
    
    // Category tabs
    int tabsHeight = 30;
    if (categoryTabs) {
        categoryTabs->setBounds(margin, topMargin, width, tabsHeight);
    }
    topMargin += tabsHeight + margin;
    
    // Preset controls
    int presetHeight = 30;
    if (presetComboBox) {
        presetComboBox->setBounds(margin, topMargin, 200, presetHeight);
    }
    if (savePresetButton) {
        savePresetButton->setBounds(210, topMargin, 60, presetHeight);
    }
    if (deletePresetButton) {
        deletePresetButton->setBounds(280, topMargin, 60, presetHeight);
    }
    if (resetButton) {
        resetButton->setBounds(350, topMargin, 60, presetHeight);
    }
    topMargin += presetHeight + margin;
    
    // Live preview controls
    int previewControlsHeight = 30;
    if (livePreviewToggle) {
        livePreviewToggle->setBounds(margin, topMargin, 100, previewControlsHeight);
    }
    if (processingIndicator) {
        processingIndicator->setBounds(110, topMargin, 100, previewControlsHeight);
    }
    if (processingLabel) {
        processingLabel->setBounds(220, topMargin, 150, previewControlsHeight);
    }
    topMargin += previewControlsHeight + margin;
    
    // AI controls
    int aiControlsHeight = 30;
    if (aiSuggestionsToggle) {
        aiSuggestionsToggle->setBounds(margin, topMargin, 100, aiControlsHeight);
    }
    if (getAISuggestionButton) {
        getAISuggestionButton->setBounds(110, topMargin, 120, aiControlsHeight);
    }
    topMargin += aiControlsHeight + margin;
    
    // Preview display (bottom half)
    int previewHeight = (bounds.getHeight() - topMargin - margin) / 2;
    if (beforeWaveform && afterWaveform) {
        beforeWaveform->setBounds(margin, topMargin, width / 2 - margin / 2, previewHeight);
        afterWaveform->setBounds(width / 2 + margin / 2, topMargin, width / 2 - margin / 2, previewHeight);
    }
    topMargin += previewHeight + margin;
    
    // Category tabs content
    if (categoryTabs) {
        categoryTabs->setBounds(margin, topMargin, width, bounds.getHeight() - topMargin - margin);
    }
    
    // Set main component size
    mainComponent->setSize(bounds.getWidth(), bounds.getHeight());
}

void SettingsPanel::registerSetting(const SettingDefinition& definition) {
    settingDefinitions[definition.id] = definition;
    
    // Set default value
    SettingValue value;
    value.value = definition.defaultValue;
    value.lastChanged = juce::Time::getCurrentTime();
    value.changedBy = "system";
    value.isDefault = true;
    
    settingValues[definition.id] = value;
}

void SettingsPanel::setSettingValue(const juce::String& id, const juce::var& value) {
    auto it = settingValues.find(id);
    if (it != settingValues.end()) {
        juce::var oldValue = it->second.value;
        it->second.value = value;
        it->second.lastChanged = juce::Time::getCurrentTime();
        it->second.changedBy = "user";
        it->second.isDefault = (value == settingDefinitions[id].defaultValue);
        
        onSettingChanged(id, value);
        notifySettingChanged(id, oldValue, value);
    }
}

juce::var SettingsPanel::getSettingValue(const juce::String& id) const {
    auto it = settingValues.find(id);
    if (it != settingValues.end()) {
        return it->second.value;
    }
    return juce::var();
}

void SettingsPanel::resetToDefaults() {
    for (auto& [id, definition] : settingDefinitions) {
        setSettingValue(id, definition.defaultValue);
    }
}

void SettingsPanel::resetToDefault(const juce::String& id) {
    auto it = settingDefinitions.find(id);
    if (it != settingDefinitions.end()) {
        setSettingValue(id, it->second.defaultValue);
    }
}

void SettingsPanel::loadPreset(const juce::String& presetName) {
    auto it = presets.find(presetName);
    if (it != presets.end()) {
        applyPreset(presetName);
        notifyPresetLoaded(presetName);
    }
}

void SettingsPanel::savePreset(const juce::String& presetName) {
    std::unordered_map<juce::String, juce::var> presetValues;
    
    for (const auto& [id, value] : settingValues) {
        presetValues[id] = value.value;
    }
    
    presets[presetName] = presetValues;
    savePresets();
    notifyPresetSaved(presetName);
}

void SettingsPanel::deletePreset(const juce::String& presetName) {
    auto it = presets.find(presetName);
    if (it != presets.end()) {
        presets.erase(it);
        savePresets();
    }
}

std::vector<juce::String> SettingsPanel::getAvailablePresets() const {
    std::vector<juce::String> presetNames;
    for (const auto& [name, values] : presets) {
        presetNames.push_back(name);
    }
    return presetNames;
}

void SettingsPanel::enableLivePreview(bool enabled) {
    livePreviewEnabled = enabled;
    
    if (livePreviewToggle) {
        livePreviewToggle->setToggleState(enabled, juce::dontSendNotification);
    }
    
    if (enabled) {
        startLivePreview();
    } else {
        stopLivePreview();
    }
}

bool SettingsPanel::isLivePreviewEnabled() const {
    return livePreviewEnabled;
}

void SettingsPanel::updatePreviewAudio(const juce::AudioBuffer<float>& audio, double sampleRate) {
    std::lock_guard<std::mutex> lock(previewMutex);
    
    currentPreview.beforeAudio = audio;
    currentPreview.sampleRate = sampleRate;
    currentPreview.isProcessing = true;
    
    if (livePreviewEnabled) {
        processPreview();
    }
}

PreviewData SettingsPanel::getPreviewData() const {
    std::lock_guard<std::mutex> lock(previewMutex);
    return currentPreview;
}

void SettingsPanel::setGrokClient(std::shared_ptr<ai::GrokAPIClient> client) {
    grokClient = client;
}

void SettingsPanel::enableAISuggestions(bool enabled) {
    aiSuggestionsEnabled = enabled;
    
    if (aiSuggestionsToggle) {
        aiSuggestionsToggle->setToggleState(enabled, juce::dontSendNotification);
    }
}

void SettingsPanel::requestAISuggestion(const juce::String& settingId) {
    if (aiSuggestionsEnabled && grokClient) {
        requestAISuggestionInternal(settingId);
    }
}

bool SettingsPanel::validateSetting(const juce::String& id, const juce::var& value) const {
    auto it = settingDefinitions.find(id);
    if (it == settingDefinitions.end()) {
        return false;
    }
    
    return validateSetting(it->second, value);
}

std::vector<juce::String> SettingsPanel::getValidationErrors() const {
    std::vector<juce::String> errors;
    
    for (const auto& [id, value] : settingValues) {
        auto defIt = settingDefinitions.find(id);
        if (defIt != settingDefinitions.end()) {
            if (!validateSetting(defIt->second, value.value)) {
                errors.push_back(id + ": Invalid value");
            }
        }
    }
    
    return errors;
}

bool SettingsPanel::exportSettings(const juce::File& filePath) const {
    SettingsPersistence persistence;
    return persistence.saveSettings(settingValues, filePath);
}

bool SettingsPanel::importSettings(const juce::File& filePath) {
    SettingsPersistence persistence;
    std::unordered_map<juce::String, SettingValue> importedValues;
    
    if (persistence.loadSettings(importedValues, filePath)) {
        for (const auto& [id, value] : importedValues) {
            auto defIt = settingDefinitions.find(id);
            if (defIt != settingDefinitions.end()) {
                setSettingValue(id, value.value);
            }
        }
        return true;
    }
    
    return false;
}

void SettingsPanel::timerCallback() {
    // Update processing indicator
    if (isProcessingPreview.load()) {
        if (processingIndicator) {
            processingIndicator->setVisible(true);
        }
        if (processingLabel) {
            processingLabel->setText("Processing...");
        }
    } else {
        if (processingIndicator) {
            processingIndicator->setVisible(false);
        }
        if (processingLabel) {
            processingLabel->setText("Ready");
        }
    }
    
    // Update preview display
    updatePreviewDisplay();
}

void SettingsPanel::addListener(Listener* listener) {
    listeners.push_back(listener);
}

void SettingsPanel::removeListener(Listener* listener) {
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

void SettingsPanel::createCategoryTabs() {
    categoryTabs = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::Orientation::horizontal);
    
    // Create tab components
    auto masteringTab = std::make_unique<juce::Component>();
    auto analysisTab = std::make_unique<juce::Component>();
    auto learningTab = std::make_unique<juce::Component>();
    auto advancedTab = std::make_unique<juce::Component>();
    
    // Add settings to tabs
    createMasteringSettings(masteringTab.get());
    createAnalysisSettings(analysisTab.get());
    createLearningSettings(learningTab.get());
    createAdvancedSettings(advancedTab.get());
    
    // Add tabs
    categoryTabs->addTab("Mastering", std::move(masteringTab), true);
    categoryTabs->addTab("Analysis", std::move(analysisTab), false);
    categoryTabs->addTab("Learning", std::move(learningTab), false);
    categoryTabs->addTab("Advanced", std::move(advancedTab), false);
    
    mainComponent->addAndMakeVisible(*categoryTabs);
}

void SettingsPanel::createSearchBar() {
    searchEditor = std::make_unique<juce::TextEditor>("Search settings...");
    searchEditor->addListener(this);
    mainComponent->addAndMakeVisible(*searchEditor);
    
    clearSearchButton = std::make_unique<juce::TextButton>("Clear");
    clearSearchButton->addListener(this);
    mainComponent->addAndMakeVisible(*clearSearchButton);
}

void SettingsPanel::createPresetControls() {
    presetComboBox = std::make_unique<juce::ComboBox>();
    presetComboBox->addListener(this);
    mainComponent->addAndMakeVisible(*presetComboBox);
    
    savePresetButton = std::make_unique<juce::TextButton>("Save");
    savePresetButton->addListener(this);
    mainComponent->addAndMakeVisible(*savePresetButton);
    
    deletePresetButton = std::make_unique<juce::TextButton>("Delete");
    deletePresetButton->addListener(this);
    mainComponent->addAndMakeVisible(*deletePresetButton);
    
    resetButton = std::make_unique<juce::TextButton>("Reset");
    resetButton->addListener(this);
    mainComponent->addAndMakeVisible(*resetButton);
    
    // Update preset list
    updatePresetList();
}

void SettingsPanel::createLivePreviewControls() {
    livePreviewToggle = std::make_unique<juce::ToggleButton>("Live Preview");
    livePreviewToggle->setToggleState(livePreviewEnabled, juce::dontSendNotification);
    livePreviewToggle->addListener(this);
    mainComponent->addAndMakeVisible(*livePreviewToggle);
    
    processingIndicator = std::make_unique<juce::ProgressBar>();
    processingIndicator->setVisible(false);
    mainComponent->addAndMakeVisible(*processingIndicator);
    
    processingLabel = std::make_unique<juce::Label>("Ready");
    processingLabel->setFont(12.0f);
    processingLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    mainComponent->addAndMakeVisible(*processingLabel);
}

void SettingsPanel::createAIControls() {
    aiSuggestionsToggle = std::make_unique<juce::ToggleButton>("AI Suggestions");
    aiSuggestionsToggle->setToggleState(aiSuggestionsEnabled, juce::dontSendNotification);
    aiSuggestionsToggle->addListener(this);
    mainComponent->addAndMakeVisible(*aiSuggestionsToggle);
    
    getAISuggestionButton = std::make_unique<juce::TextButton>("Get AI Suggestion");
    getAISuggestionButton->addListener(this);
    mainComponent->addAndMakeVisible(*getAISuggestionButton);
}

void SettingsPanel::createPreviewDisplay() {
    beforeWaveform = std::make_unique<WaveformContainer>();
    afterWaveform = std::make_unique<WaveformContainer>();
    
    beforeLabel = std::make_unique<juce::Label>("Before");
    beforeLabel->setFont(14.0f);
    beforeLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    beforeLabel->setJustificationType(juce::Justification::centred);
    
    afterLabel = std::make_unique<juce::Label>("After");
    afterLabel->setFont(14.0f);
    afterLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    afterLabel->setJustificationType(juce::Justification::centred);
    
    mainComponent->addAndMakeVisible(*beforeWaveform);
    mainComponent->addAndMakeVisible(*afterWaveform);
    mainComponent->addAndMakeVisible(*beforeLabel);
    mainComponent->addAndMakeVisible(*afterLabel);
}

void SettingsPanel::createSettingComponents() {
    // Components are created in category-specific methods
}

void SettingsPanel::createMasteringSettings(juce::Component* parent) {
    int x = 10;
    int y = 10;
    int width = 200;
    int height = 30;
    int spacing = 40;
    
    // Add mastering settings
    for (const auto& [id, definition] : settingDefinitions) {
        if (definition.category == SettingCategory::Mastering) {
            std::unique_ptr<juce::Component> component;
            
            switch (definition.type) {
                case SettingType::Slider:
                    component = createSlider(definition);
                    break;
                case SettingType::Toggle:
                    component = createToggle(definition);
                    break;
                case SettingType::ComboBox:
                    component = createComboBox(definition);
                    break;
                case SettingType::Text:
                    component = createTextEditor(definition);
                    break;
                default:
                    continue;
            }
            
            component->setBounds(x, y, width, height);
            parent->addAndMakeVisible(*component);
            
            settingComponents[id] = std::move(component);
            y += height + spacing;
        }
    }
}

void SettingsPanel::createAnalysisSettings(juce::Component* parent) {
    // Similar to createMasteringSettings but for analysis settings
    createMasteringSettings(parent);  // Placeholder - would filter by category
}

void SettingsPanel::createLearningSettings(juce::Component* parent) {
    // Similar to createMasteringSettings but for learning settings
    createMasteringSettings(parent);  // Placeholder - would filter by category
}

void SettingsPanel::createAdvancedSettings(juce::Component* parent) {
    // Similar to createMasteringSettings but for advanced settings
    createMasteringSettings(parent);  // Placeholder - would filter by category
}

std::unique_ptr<juce::Slider> SettingsPanel::createSlider(const SettingDefinition& def) {
    auto slider = std::make_unique<juce::Slider>(def.name);
    
    float minVal = def.minValue.isDouble() ? static_cast<float>(def.minValue.getDouble()) : 0.0f;
    float maxVal = def.maxValue.isDouble() ? static_cast<float>(def.maxValue.getDouble()) : 100.0f;
    float defaultVal = def.defaultValue.isDouble() ? static_cast<float>(def.defaultValue.getDouble()) : 50.0f;
    
    slider->setRange(minVal, maxVal, 0.1f);
    slider->setValue(defaultVal);
    slider->setSliderStyle(juce::Slider::LinearHorizontal);
    slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    slider->addListener(this);
    
    return slider;
}

std::unique_ptr<juce::ToggleButton> SettingsPanel::createToggle(const SettingDefinition& def) {
    auto toggle = std::make_unique<juce::ToggleButton>(def.name);
    
    bool defaultVal = def.defaultValue.isBool() ? def.defaultValue.getBool() : false;
    toggle->setToggleState(defaultVal);
    toggle->addListener(this);
    
    return toggle;
}

std::unique_ptr<juce::ComboBox> SettingsPanel::createComboBox(const SettingDefinition& def) {
    auto comboBox = std::make_unique<juce::ComboBox>(def.name);
    
    for (const auto& option : def.options) {
        comboBox->addItem(option);
    }
    
    if (!def.options.empty()) {
        comboBox->setSelectedId(1);
    }
    
    comboBox->addListener(this);
    
    return comboBox;
}

std::unique_ptr<juce::TextEditor> SettingsPanel::createTextEditor(const SettingDefinition& def) {
    auto editor = std::make_unique<juce::TextEditor>(def.name);
    
    juce::String defaultVal = def.defaultValue.toString();
    editor->setText(defaultVal);
    editor->addListener(this);
    
    return editor;
}

void SettingsPanel::onSettingChanged(const juce::String& id, const juce::var& newValue) {
    // Trigger live preview if enabled
    if (livePreviewEnabled) {
        startLivePreview();
    }
}

void SettingsPanel::onSliderChanged(juce::Slider* slider) {
    // Find the setting ID for this slider
    for (const auto& [id, component] : settingComponents) {
        if (component.get() == slider) {
            setSettingValue(id, slider->getValue());
            break;
        }
    }
}

void SettingsPanel::onToggleChanged(juce::ToggleButton* toggle) {
    // Find the setting ID for this toggle
    for (const auto& [id, component] : settingComponents) {
        if (component.get() == toggle) {
            setSettingValue(id, toggle->getToggleState());
            break;
        }
    }
}

void SettingsPanel::onComboBoxChanged(juce::ComboBox* comboBox) {
    // Find the setting ID for this combo box
    for (const auto& [id, component] : settingComponents) {
        if (component.get() == comboBox) {
            juce::var value = comboBox->getText();
            setSettingValue(id, value);
            break;
        }
    }
}

void SettingsPanel::onButtonClicked(juce::Button* button) {
    if (button == clearSearchButton.get()) {
        searchEditor->setText("");
        clearFilter();
    } else if (button == savePresetButton.get()) {
        juce::String presetName = "Custom " + juce::String::formatted("%02d", juce::Time::getCurrentTime().getSeconds());
        savePreset(presetName);
    } else if (button == deletePresetButton.get()) {
        juce::String selectedPreset = presetComboBox->getText();
        if (!selectedPreset.isEmpty()) {
            deletePreset(selectedPreset);
        }
    } else if (button == resetButton.get()) {
        resetToDefaults();
    } else if (button == getAISuggestionButton.get()) {
        // Get suggestion for currently selected setting
        requestAISuggestion("current_setting");
    }
}

void SettingsPanel::onTextEditorChanged(juce::TextEditor* editor) {
    // Find the setting ID for this text editor
    for (const auto& [id, component] : settingComponents) {
        if (component.get() == editor) {
            setSettingValue(id, editor->getText());
            break;
        }
    }
}

void SettingsPanel::startLivePreview() {
    isProcessingPreview.store(true);
    
    // Process preview in background thread using managed thread
    if (!previewThread.isThreadRunning()) {
        previewThread.startThread();
    }
}

void SettingsPanel::stopLivePreview() {
    isProcessingPreview.store(false);
}

void SettingsPanel::processPreview() {
    std::lock_guard<std::mutex> lock(previewMutex);
    
    // Apply current settings to create "after" audio
    // This is a simplified implementation
    currentPreview.afterAudio = currentPreview.beforeAudio;
    
    // Apply processing based on current settings
    float intensity = getSettingValue("intensity").isDouble() ? 
        static_cast<float>(getSettingValue("intensity").getDouble()) : 0.5f;
    
    // Apply gain adjustment
    currentPreview.afterAudio.applyGain(intensity);
    
    // Calculate processing time
    auto startTime = std::chrono::steady_clock::now();
    
    // Simulate processing delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto endTime = std::chrono::steady_clock::now();
    auto processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    currentPreview.processingTime = juce::String(processingTime.count()) + "ms";
    
    // Calculate quality metrics (simplified)
    float beforeRMS = 0.0f;
    float afterRMS = 0.0f;
    
    for (int ch = 0; ch < currentPreview.beforeAudio.getNumChannels(); ++ch) {
        for (int i = 0; i < currentPreview.beforeAudio.getNumSamples(); ++i) {
            beforeRMS += currentPreview.beforeAudio.getSample(ch, i) * currentPreview.beforeAudio.getSample(ch, i);
            afterRMS += currentPreview.afterAudio.getSample(ch, i) * currentPreview.afterAudio.getSample(ch, i);
        }
    }
    
    beforeRMS = std::sqrt(beforeRMS / (currentPreview.beforeAudio.getNumSamples() * currentPreview.beforeAudio.getNumChannels()));
    afterRMS = std::sqrt(afterRMS / (currentPreview.afterAudio.getNumSamples() * currentPreview.afterAudio.getNumChannels()));
    
    float improvement = juce::Decibels::gainToDecibels(afterRMS) - juce::Decibels::gainToDecibels(beforeRMS);
    currentPreview.qualityMetrics = "Improvement: " + juce::String(improvement, 1) + "dB";
    
    currentPreview.isProcessing = false;
    
    notifyPreviewUpdated(currentPreview);
}

void SettingsPanel::updatePreviewDisplay() {
    if (beforeWaveform && afterWaveform) {
        std::lock_guard<std::mutex> lock(previewMutex);
        
        beforeWaveform->setAudioData(currentPreview.beforeAudio, currentPreview.sampleRate);
        afterWaveform->setAudioData(currentPreview.afterAudio, currentPreview.sampleRate);
    }
}

void SettingsPanel::requestAISuggestionInternal(const juce::String& settingId) {
    if (!grokClient) return;
    
    // Build prompt for AI suggestion
    juce::String prompt = "Suggest an optimal value for the setting '" + settingId + "' ";
    prompt += "based on current audio characteristics and mastering best practices.";
    
    // Call Grok API
    auto response = grokClient->callGrok(prompt);
    
    if (response.isNotEmpty()) {
        // Parse response and apply suggestion
        // This is simplified - would need proper parsing
        juce::var suggestedValue = response.getFloatValue();  // Simplified
        applyAISuggestion(settingId, suggestedValue);
    }
}

void SettingsPanel::applyAISuggestion(const juce::String& settingId, const juce::var& suggestedValue) {
    setSettingValue(settingId, suggestedValue);
    
    // Show notification
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
        "AI Suggestion Applied",
        "AI suggested value for " + settingId + ": " + suggestedValue.toString(),
        "OK");
}

void SettingsPanel::loadPresets() {
    presetsFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                     .getChildFile("ZenithDAW")
                     .getChildFile("settings_presets.json");
    
    SettingsPersistence persistence;
    std::unordered_map<juce::String, std::unordered_map<juce::String, juce::var>> loadedPresets;
    
    if (persistence.loadFromJSON(loadedPresets, presetsFile)) {
        presets = loadedPresets;
    }
    
    // Add built-in presets if none exist
    if (presets.empty()) {
        presets["Default"] = {};
        presets["Warm"] = {{"character", 0.7f}, {"intensity", 0.3f}};
        presets["Bright"] = {{"character", 0.3f}, {"intensity", 0.7f}};
        presets["Aggressive"] = {{"intensity", 0.9f}, {"character", 0.5f}};
        
        savePresets();
    }
    
    updatePresetList();
}

void SettingsPanel::savePresets() {
    SettingsPersistence persistence;
    persistence.saveAsJSON(presets, presetsFile);
}

void SettingsPanel::applyPreset(const juce::String& presetName) {
    auto it = presets.find(presetName);
    if (it != presets.end()) {
        for (const auto& [id, value] : it->second) {
            setSettingValue(id, value);
        }
    }
}

void SettingsPanel::updatePresetList() {
    if (presetComboBox) {
        presetComboBox->clear();
        
        for (const auto& [name, values] : presets) {
            presetComboBox->addItem(name);
        }
    }
}

bool SettingsPanel::validateSetting(const SettingDefinition& def, const juce::var& value) const {
    switch (def.type) {
        case SettingType::Slider:
            return validateSliderValue(def, static_cast<float>(value.getDouble()));
        case SettingType::Text:
            return validateTextValue(def, value.toString());
        default:
            return true;
    }
}

bool SettingsPanel::validateSliderValue(const SettingDefinition& def, float value) const {
    float minVal = def.minValue.isDouble() ? static_cast<float>(def.minValue.getDouble()) : 0.0f;
    float maxVal = def.maxValue.isDouble() ? static_cast<float>(def.maxValue.getDouble()) : 100.0f;
    
    return value >= minVal && value <= maxVal;
}

bool SettingsPanel::validateTextValue(const SettingDefinition& def, const juce::String& value) const {
    // Basic validation - would be more sophisticated in production
    return !value.isEmpty();
}

// SettingsPanelFactory Implementation
std::unique_ptr<SettingsPanel> SettingsPanelFactory::createDefaultPanel() {
    return std::make_unique<SettingsPanel>();
}

std::unique_ptr<SettingsPanel> SettingsPanelFactory::createMinimalPanel() {
    auto panel = std::make_unique<SettingsPanel>();
    // Would filter settings for minimal panel
    return panel;
}

std::unique_ptr<SettingsPanel> SettingsPanelFactory::createAdvancedPanel() {
    auto panel = std::make_unique<SettingsPanel>();
    // Would add advanced settings for advanced panel
    return panel;
}

std::vector<SettingDefinition> SettingsPanelFactory::getDefaultMasteringSettings() {
    return {
        {"intensity", "Intensity", "Overall processing intensity", SettingType::Slider, SettingCategory::Mastering, 0.5, 0.0, 1.0, "", true, false},
        {"target_loudness", "Target Loudness", "Target output loudness in LUFS", SettingType::Slider, SettingCategory::Mastering, -10.0, -14.0, -6.0, "dB", true, false},
        {"character", "Character", "Processing character", SettingType::Slider, SettingCategory::Mastering, 0.5, 0.0, 1.0, "", true, false},
        {"creativity", "Creativity", "AI creativity level", SettingType::Slider, SettingCategory::Mastering, 0.3, 0.0, 1.0, "", true, false},
        {"mastering_mode", "Mode", "Mastering mode", SettingType::ComboBox, SettingCategory::Mastering, "balanced", juce::var(), juce::var(), "", true, false, {"gentle", "balanced", "aggressive", "vintage", "modern"}},
        {"bypass", "Bypass", "Bypass processing", SettingType::Toggle, SettingCategory::Mastering, false, juce::var(), juce::var(), "", true, false}
    };
}

std::vector<SettingDefinition> SettingsPanelFactory::getDefaultAnalysisSettings() {
    return {
        {"realtime_analysis", "Real-time Analysis", "Enable real-time analysis", SettingType::Toggle, SettingCategory::Analysis, true, juce::var(), juce::var(), "", true, false},
        {"auto_detect_genre", "Auto-detect Genre", "Automatically detect music genre", SettingType::Toggle, SettingCategory::Analysis, true, juce::var(), juce::var(), "", true, false},
        {"show_spectrum", "Show Spectrum", "Display spectrum analysis", SettingType::Toggle, SettingCategory::Analysis, false, juce::var(), juce::var(), "", true, false},
        {"show_phase", "Show Phase", "Display phase analysis", SettingType::Toggle, SettingCategory::Analysis, false, juce::var(), juce::var(), "", true, false}
    };
}

std::vector<SettingDefinition> SettingsPanelFactory::getDefaultLearningSettings() {
    return {
        {"auto_learn", "Auto Learn", "Enable automatic learning", SettingType::Toggle, SettingCategory::Learning, false, juce::var(), juce::var(), "", true, false},
        {"learning_rate", "Learning Rate", "Learning rate for neural networks", SettingType::Slider, SettingCategory::Learning, 0.01f, 0.001f, 0.1f, "", true, false},
        {"batch_size", "Batch Size", "Training batch size", SettingType::Slider, SettingCategory::Learning, 32.0f, 1.0f, 128.0f, "", true, false},
        {"epochs", "Epochs", "Training epochs", SettingType::Slider, SettingCategory::Learning, 100.0f, 10.0f, 1000.0f, "", true, false}
    };
}

std::vector<SettingDefinition> SettingsPanelFactory::getDefaultAdvancedSettings() {
    return {
        {"gpu_acceleration", "GPU Acceleration", "Enable GPU acceleration", SettingType::Toggle, SettingCategory::Advanced, false, juce::var(), juce::var(), "", true, false},
        {"max_memory", "Max Memory", "Maximum memory usage (MB)", SettingType::Slider, SettingCategory::Advanced, 512.0f, 128.0f, 2048.0f, "MB", true, false},
        {"thread_count", "Thread Count", "Number of processing threads", SettingType::Slider, SettingCategory::Advanced, 4.0f, 1.0f, 16.0f, "", true, false},
        {"debug_mode", "Debug Mode", "Enable debug output", SettingType::Toggle, SettingCategory::Advanced, false, juce::var(), juce::var(), "", true, false}
    };
}

} // namespace ui
} // namespace zenith
