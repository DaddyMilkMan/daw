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

#include "../../design-system/ZenithTheme.h"
#include "../../engine/Engine.h"
#include "ai/GrokAPIClient.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
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
    juce::String placeholder;
    std::vector<juce::String> options; // For combo box
    std::function<void(juce::var)> onChange; // Callback for changes
};

// Value for a setting
struct SettingValue {
    juce::String id;
    juce::var value;
    bool hasChanges = false;
};

/**
 * @class SkiaSettingsPanel
 * @brief Skia-based settings panel with live preview and AI suggestions
 *
 * Features:
 * - Glassmorphic panel design
 * - Tab-based category navigation
 * - Live preview panel (before/after waveforms)
 * - AI suggestion integration
 * - Preset save/load
 * - Import/export settings
 * - Category-based organization
 * - Real-time validation
 */
class SkiaSettingsPanel : public SkiaComponent {
public:
    //==========================================================================
    explicit SkiaSettingsPanel(Engine& engine);
    ~SkiaSettingsPanel() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void drawSkia(SkCanvas* canvas) override;
    void resized() override;

    //==========================================================================
    // Settings management
    //==========================================================================

    /**
     * @brief Load settings from project/saved state
     */
    void loadSettings(const std::vector<SettingValue>& settings);

    /**
     * @brief Save current settings to project/saved state
     */
    std::vector<SettingValue> saveSettings() const;

    /**
     * @brief Add a new setting definition
     */
    void addSetting(const SettingDefinition& definition);

    /**
     * @brief Get a setting value by ID
     */
    juce::var getSettingValue(const juce::String& id) const;

    /**
     * @brief Set a setting value by ID
     */
    void setSettingValue(const juce::String& id, const juce::var& value);

    /**
     * @brief Reset to default values
     */
    void resetToDefaults();

    /**
     * @brief Export current settings to file
     */
    void exportSettings(const juce::File& file);

    /**
     * @brief Import settings from file
     */
    void importSettings(const juce::File& file);

    //==========================================================================
    // Callbacks
    //==========================================================================

    std::function<void(const juce::String&)> onSettingChanged;
    std::function<void(const juce::String&)> onPresetLoaded;
    std::function<void()> onExportRequested;
    std::function<void()> onImportRequested;

private:
    //==========================================================================
    // Types
    //==========================================================================

    struct TabButton {
        SettingCategory category;
        juce::String label;
        SkRect bounds;
        bool isActive = false;
        bool isHovered = false;
    };

    struct SettingRow {
        SettingDefinition definition;
        juce::var currentValue;
        SkRect bounds;
        bool isHovered = false;
        juce::Component* editor = nullptr; // For complex editors
    };

    //==========================================================================
    // Layout Constants
    //==========================================================================

    static constexpr float kHeaderHeight = 80.0f;
    static constexpr float kTabHeight = 50.0f;
    static constexpr float kPreviewHeight = 200.0f;
    static constexpr float kFooterHeight = 80.0f;
    static constexpr float kPanelPadding = 24.0f;
    static constexpr float kSettingSpacing = 16.0f;
    static constexpr float kTabSpacing = 8.0f;

    //==========================================================================
    // Drawing Methods
    //==========================================================================

    void drawHeader(SkCanvas* canvas, const SkRect& bounds);
    void drawTabs(SkCanvas* canvas, const SkRect& bounds);
    void drawPreviewPanel(SkCanvas* canvas, const SkRect& bounds);
    void drawSettingsList(SkCanvas* canvas, const SkRect& bounds);
    void drawFooter(SkCanvas* canvas, const SkRect& bounds);

    void drawTabButton(SkCanvas* canvas, const TabButton& button);
    void drawSettingRow(SkCanvas* canvas, const SettingRow& row);

    // Individual setting renderers
    void drawSliderSetting(SkCanvas* canvas, const SettingRow& row);
    void drawToggleSetting(SkCanvas* canvas, const SettingRow& row);
    void drawComboBoxSetting(SkCanvas* canvas, const SettingRow& row);
    void drawTextSetting(SkCanvas* canvas, const SettingRow& row);
    void drawColorSetting(SkCanvas* canvas, const SettingRow& row);
    void drawFileSetting(SkCanvas* canvas, const SettingRow& row);
    void drawButtonSetting(SkCanvas* canvas, const SettingRow& row);

    //==========================================================================
    // Input Handling
    //==========================================================================

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;

    //==========================================================================
    // Helper Methods
    //==========================================================================

    void updateLayout();
    void selectCategory(SettingCategory category);
    void updatePreview();
    void applySettings();
    juce::String getCategoryName(SettingCategory category) const;
    SkColor getCategoryColor(SettingCategory category) const;
    void showAIRecommendations();
    void validateSetting(const SettingRow& row);

    //==========================================================================
    // Member variables
    //==========================================================================

    Engine& engine;

    // UI State
    std::vector<SettingDefinition> settingDefinitions_;
    std::vector<SettingRow> settingRows_;
    std::vector<TabButton> tabButtons_;
    SettingCategory selectedCategory_ = SettingCategory::Mastering;
    juce::String searchText_;

    // Settings values
    std::vector<SettingValue> currentValues_;
    std::vector<SettingValue> defaultValues_;

    // Layout
    SkRect headerRect_;
    SkRect tabsRect_;
    SkRect previewRect_;
    SkRect settingsRect_;
    SkRect footerRect_;

    // AI integration
    std::unique_ptr<zenith::GrokAPIClient> grokClient_;
    juce::String aiSuggestions_;
    bool aiSuggestionsVisible_ = false;

    // State
    bool hasUnsavedChanges_ = false;
    juce::File lastExportFile_;
    juce::File lastImportFile_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaSettingsPanel)
};

} // namespace ui
} // namespace zenith