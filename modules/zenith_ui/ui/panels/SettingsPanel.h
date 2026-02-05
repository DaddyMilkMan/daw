/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================
    SettingsPanel.h
    Settings panel with live preview - production ready
    Phase 4: User Interface
  ==============================================================================
*/


#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../visualizations/WaveformDisplay.h"
#include "../../ai/GrokAPIClient.h"
#include <memory>
#include <vector>

namespace zenith {
namespace ui {

// Setting categories
enum class SettingCategory {
    Mastering,
    Analysis,
    Learning,
    Advanced,
    Presets
};

// Setting types
enum class SettingType {
    Slider,
    Toggle,
    ComboBox,
    Text,
    Color,
    File,
    Button
};

// Setting definition
struct SettingDefinition {
    juce::String id;
    juce::String name;
    juce::String description;
    SettingType type;
    SettingCategory category;
    juce::var defaultValue;
    juce::var minValue;
    juce::var maxValue;
    juce::String unit;
    bool isAutomatable;
    bool requiresRestart;
    std::vector<juce::String> options;  // For ComboBox
};

// Setting value with metadata
struct SettingValue {
    juce::var value;
    juce::Time lastChanged;
    juce::String changedBy;  // "user", "ai", "preset"
    bool isDefault;
};

// Live preview data
struct PreviewData {
    juce::AudioBuffer<float> beforeAudio;
    juce::AudioBuffer<float> afterAudio;
    double sampleRate;
    juce::String processingTime;
    juce::String qualityMetrics;
    bool isProcessing;
};

// Settings panel with live preview
class SettingsPanel : public juce::Component,
                     public juce::Slider::Listener,
                     public juce::ToggleButton::Listener,
                     public juce::ComboBox::Listener,
                     public juce::Button::Listener,
                     public juce::TextEditor::Listener,
                     public juce::Timer {
public:
    SettingsPanel();
    ~SettingsPanel() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Settings management
    void registerSetting(const SettingDefinition& definition);
    void setSettingValue(const juce::String& id, const juce::var& value);
    juce::var getSettingValue(const juce::String& id) const;
    void resetToDefaults();
    void resetToDefault(const juce::String& id);
    
    // Presets
    void loadPreset(const juce::String& presetName);
    void savePreset(const juce::String& presetName);
    void deletePreset(const juce::String& presetName);
    std::vector<juce::String> getAvailablePresets() const;
    
    // Live preview
    void enableLivePreview(bool enabled);
    bool isLivePreviewEnabled() const;
    void updatePreviewAudio(const juce::AudioBuffer<float>& audio, double sampleRate);
    PreviewData getPreviewData() const;
    
    // AI integration
    void setGrokClient(std::shared_ptr<ai::GrokAPIClient> client);
    void enableAISuggestions(bool enabled);
    void requestAISuggestion(const juce::String& settingId);
    
    // Validation
    bool validateSetting(const juce::String& id, const juce::var& value) const;
    std::vector<juce::String> getValidationErrors() const;
    
    // Import/Export
    bool exportSettings(const juce::File& filePath) const;
    bool importSettings(const juce::File& filePath);
    
    // Search and filter
    void filterByCategory(SettingCategory category);
    void searchSettings(const juce::String& searchTerm);
    void clearFilter();
    
    // Timer callback for live updates
    void timerCallback() override;
    
    // Listeners
    struct Listener {
        virtual ~Listener() = default;
        virtual void settingChanged(const juce::String& id, const juce::var& oldValue, const juce::var& newValue) {}
        virtual void presetLoaded(const juce::String& presetName) {}
        virtual void presetSaved(const juce::String& presetName) {}
        virtual void previewUpdated(const PreviewData& preview) {}
        virtual void validationError(const juce::String& id, const juce::String& error) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    // Settings storage
    std::unordered_map<juce::String, SettingDefinition> settingDefinitions;
    std::unordered_map<juce::String, SettingValue> settingValues;
    
    // UI components
    std::unique_ptr<juce::Viewport> mainViewport;
    std::unique_ptr<juce::Component> mainComponent;
    
    // Category tabs
    std::unique_ptr<juce::TabbedComponent> categoryTabs;
    
    // Search bar
    std::unique_ptr<juce::TextEditor> searchEditor;
    std::unique_ptr<juce::TextButton> clearSearchButton;
    
    // Preset controls
    std::unique_ptr<juce::ComboBox> presetComboBox;
    std::unique_ptr<juce::TextButton> savePresetButton;
    std::unique_ptr<juce::TextButton> deletePresetButton;
    std::unique_ptr<juce::TextButton> resetButton;
    
    // Live preview controls
    std::unique_ptr<juce::ToggleButton> livePreviewToggle;
    std::unique_ptr<juce::ProgressBar> processingIndicator;
    std::unique_ptr<juce::Label> processingLabel;
    
    // AI controls
    std::unique_ptr<juce::ToggleButton> aiSuggestionsToggle;
    std::unique_ptr<juce::TextButton> getAISuggestionButton;
    
    // Preview display
    std::unique_ptr<WaveformContainer> beforeWaveform;
    std::unique_ptr<WaveformContainer> afterWaveform;
    std::unique_ptr<juce::Label> beforeLabel;
    std::unique_ptr<juce::Label> afterLabel;
    
    // Settings components (created dynamically)
    std::unordered_map<juce::String, std::unique_ptr<juce::Component>> settingComponents;
    
    // State
    bool livePreviewEnabled = true;
    bool aiSuggestionsEnabled = false;
    SettingCategory currentFilter = SettingCategory::Mastering;
    std::shared_ptr<ai::GrokAPIClient> grokClient;
    
    // Preview data
    std::atomic<bool> isProcessingPreview{false};
    PreviewData currentPreview;
    std::mutex previewMutex;
    
    // Managed thread for preview processing
    class PreviewThread : public juce::Thread {
    public:
        PreviewThread(SettingsPanel& panel) : juce::Thread("SettingsPreview"), owner(panel) {}
        
        void run() override {
            while (!threadShouldExit()) {
                if (owner.isProcessingPreview.load()) {
                    owner.processPreview();
                    owner.isProcessingPreview.store(false);
                }
                wait(100); // Check every 100ms
            }
        }
        
    private:
        SettingsPanel& owner;
    };
    
    PreviewThread previewThread{*this};
    
    // Listeners
    std::vector<Listener*> listeners;
    
    // Preset management
    std::unordered_map<juce::String, std::unordered_map<juce::String, juce::var>> presets;
    juce::File presetsFile;
    
    // UI creation
    void createCategoryTabs();
    void createSearchBar();
    void createPresetControls();
    void createLivePreviewControls();
    void createAIControls();
    void createPreviewDisplay();
    void createSettingComponents();
    
    // Category-specific UI creation
    void createMasteringSettings(juce::Component* parent);
    void createAnalysisSettings(juce::Component* parent);
    void createLearningSettings(juce::Component* parent);
    void createAdvancedSettings(juce::Component* parent);
    
    // Component creation helpers
    std::unique_ptr<juce::Slider> createSlider(const SettingDefinition& def);
    std::unique_ptr<juce::ToggleButton> createToggle(const SettingDefinition& def);
    std::unique_ptr<juce::ComboBox> createComboBox(const SettingDefinition& def);
    std::unique_ptr<juce::TextEditor> createTextEditor(const SettingDefinition& def);
    std::unique_ptr<juce::ColourSelector> createColorSelector(const SettingDefinition& def);
    std::unique_ptr<juce::FileChooser> createFileChooser(const SettingDefinition& def);
    std::unique_ptr<juce::TextButton> createButton(const SettingDefinition& def);
    
    // Event handlers
    void onSettingChanged(const juce::String& id, const juce::var& newValue);
    void onSliderChanged(juce::Slider* slider);
    void onToggleChanged(juce::ToggleButton* toggle);
    void onComboBoxChanged(juce::ComboBox* comboBox);
    void onButtonClicked(juce::Button* button);
    void onTextEditorChanged(juce::TextEditor* editor);
    
    // Live preview
    void startLivePreview();
    void stopLivePreview();
    void processPreview();
    void updatePreviewDisplay();
    
    // AI integration
    void requestAISuggestionInternal(const juce::String& settingId);
    void applyAISuggestion(const juce::String& settingId, const juce::var& suggestedValue);
    
    // Validation
    bool validateSliderValue(const SettingDefinition& def, float value) const;
    bool validateTextValue(const SettingDefinition& def, const juce::String& value) const;
    bool validateColorValue(const SettingDefinition& def, const juce::Colour& color) const;
    
    // Preset management
    void loadPresets();
    void savePresets();
    void applyPreset(const juce::String& presetName);
    
    // Notification
    void notifySettingChanged(const juce::String& id, const juce::var& oldValue, const juce::var& newValue);
    void notifyPresetLoaded(const juce::String& presetName);
    void notifyPresetSaved(const juce::String& presetName);
    void notifyPreviewUpdated(const PreviewData& preview);
    void notifyValidationError(const juce::String& id, const juce::String& error);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPanel)
};

// Settings panel factory
class SettingsPanelFactory {
public:
    static std::unique_ptr<SettingsPanel> createDefaultPanel();
    static std::unique_ptr<SettingsPanel> createMinimalPanel();
    static std::unique_ptr<SettingsPanel> createAdvancedPanel();
    
    // Default settings definitions
    static std::vector<SettingDefinition> getDefaultMasteringSettings();
    static std::vector<SettingDefinition> getDefaultAnalysisSettings();
    static std::vector<SettingDefinition> getDefaultLearningSettings();
    static std::vector<SettingDefinition> getDefaultAdvancedSettings();
    
private:
    static void addCommonSettings(std::vector<SettingDefinition>& settings);
};

// Settings validator
class SettingsValidator {
public:
    struct ValidationRule {
        juce::String id;
        juce::String description;
        std::function<bool(const juce::var&)> validator;
        juce::String errorMessage;
    };
    
    SettingsValidator();
    ~SettingsValidator() = default;
    
    // Rule management
    void addRule(const ValidationRule& rule);
    void removeRule(const juce::String& id);
    void clearRules();
    
    // Validation
    bool validateSetting(const SettingDefinition& def, const juce::var& value) const;
    std::vector<juce::String> getValidationErrors(const SettingDefinition& def, const juce::var& value) const;
    
    // Default rules
    void addDefaultRules();
    
private:
    std::vector<ValidationRule> rules;
    
    // Default validators
    static bool validateRange(const juce::var& value, float minVal, float maxVal);
    static bool validatePositive(const juce::var& value);
    static bool validatePercentage(const juce::var& value);
    static bool validateFrequency(const juce::var& value);
    static bool validateTime(const juce::var& value);
    static bool validateColor(const juce::var& value);
};

// Settings persistence
class SettingsPersistence {
public:
    SettingsPersistence();
    ~SettingsPersistence() = default;
    
    // File operations
    bool saveSettings(const std::unordered_map<juce::String, SettingValue>& settings, const juce::File& file) const;
    bool loadSettings(std::unordered_map<juce::String, SettingValue>& settings, const juce::File& file) const;
    
    // Format support
    bool saveAsJSON(const std::unordered_map<juce::String, SettingValue>& settings, const juce::File& file) const;
    bool loadFromJSON(std::unordered_map<juce::String, SettingValue>& settings, const juce::File& file) const;
    bool saveAsXML(const std::unordered_map<juce::String, SettingValue>& settings, const juce::File& file) const;
    bool loadFromXML(std::unordered_map<juce::String, SettingValue>& settings, const juce::File& file) const;
    
    // Backup and restore
    bool createBackup(const std::unordered_map<juce::String, SettingValue>& settings, const juce::File& backupDir) const;
    bool restoreFromBackup(std::unordered_map<juce::String, SettingValue>& settings, const juce::File& backupDir) const;
    
private:
    juce::String formatValue(const juce::var& value) const;
    juce::var parseValue(const juce::String& str, const juce::var& defaultValue) const;
    
    juce::String serializeTime(const juce::Time& time) const;
    juce::Time deserializeTime(const juce::String& str) const;
};

} // namespace ui
} // namespace zenith
