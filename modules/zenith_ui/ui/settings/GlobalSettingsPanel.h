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

#pragma once

#include "../framework/SkiaComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace zenith {

/**
 * @class GlobalSettingsPanel
 * @brief Global application settings management panel
 * 
 * This panel provides access to application-wide settings that affect
 * all projects and the overall user experience.
 */
class GlobalSettingsPanel : public SkiaComponent {
public:
    //==========================================================================
    // Construction
    //==========================================================================
    
    GlobalSettingsPanel();
    ~GlobalSettingsPanel() override;
    
    //==========================================================================
    // SkiaComponent Overrides
    //==========================================================================
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    
    //==========================================================================
    // Settings Management
    //==========================================================================
    
    /**
     * @brief Load all settings from disk
     */
    void loadSettings();
    
    /**
     * @brief Save all settings to disk
     */
    void saveSettings();
    
    /**
     * @brief Reset all settings to factory defaults
     */
    void resetToDefaults();
    
    //==========================================================================
    // Callbacks
    //==========================================================================
    
    std::function<void()> onSettingsChanged;
    std::function<void()> onRequestClose;

private:
    //==========================================================================
    // Settings Categories
    //==========================================================================
    
    enum class Section {
        General,
        Projects,
        Privacy,
        Network,
        Account,
        Count
    };
    
    //==========================================================================
    // Setting Types
    //==========================================================================
    
    struct SettingItem {
        juce::String id;
        juce::String label;
        juce::String description;
        juce::var value;
        juce::var defaultValue;
        
        enum Type {
            Boolean,
            Integer,
            String,
            Path,
            Choice,
            Button
        } type;
        
        juce::StringArray choices;  // For Choice type
        int minValue = 0, maxValue = 100;  // For Integer type
        
        // UI State
        juce::Rectangle<float> bounds;
        bool isHovered = false;
        bool isEditing = false;
        
        // Callbacks
        std::function<void(const juce::var&)> onChanged;
        std::function<void()> onClick;
    };
    
    struct SettingsSection {
        juce::String name;
        juce::String icon;
        std::vector<SettingItem> items;
    };
    
    //==========================================================================
    // Layout Constants
    //==========================================================================
    
    static constexpr float kSidebarWidth = 200.0f;
    static constexpr float kHeaderHeight = 60.0f;
    static constexpr float kItemHeight = 56.0f;
    static constexpr float kItemSpacing = 8.0f;
    static constexpr float kPadding = 20.0f;
    static constexpr float kCornerRadius = 12.0f;
    
    //==========================================================================
    // State
    //==========================================================================
    
    Section currentSection_ = Section::General;
    std::vector<SettingsSection> sections_;
    
    // UI State
    int hoveredItemIndex_ = -1;
    int hoveredSectionIndex_ = -1;
    float animationProgress_ = 1.0f;
    
    // Text editor for string inputs
    std::unique_ptr<juce::TextEditor> textEditor_;
    SettingItem* editingItem_ = nullptr;
    
    //==========================================================================
    // Initialization
    //==========================================================================
    
    void initializeSections();
    void setupGeneralSection();
    void setupProjectsSection();
    void setupPrivacySection();
    void setupNetworkSection();
    void setupAccountSection();
    
    //==========================================================================
    // Drawing
    //==========================================================================
    
    void drawBackground(SkCanvas* canvas);
    void drawHeader(SkCanvas* canvas);
    void drawSidebar(SkCanvas* canvas);
    void drawContent(SkCanvas* canvas);
    void drawSettingItem(SkCanvas* canvas, const SettingItem& item, float y);
    void drawSectionButton(SkCanvas* canvas, const SettingsSection& section, 
                          int index, float y, bool isActive);
    void drawBooleanControl(SkCanvas* canvas, const SettingItem& item, float x, float y, float width);
    void drawChoiceControl(SkCanvas* canvas, const SettingItem& item, float x, float y, float width);
    void drawPathControl(SkCanvas* canvas, const SettingItem& item, float x, float y, float width);
    void drawButtonControl(SkCanvas* canvas, const SettingItem& item, float x, float y, float width);
    
    //==========================================================================
    // Input Handling
    //==========================================================================
    
    void handleSectionClick(Section section);
    void handleItemClick(int itemIndex);
    void handleItemValueClick(int itemIndex);
    void updateHoverState(juce::Point<int> pos);
    
    void startEditingItem(SettingItem* item);
    void stopEditingItem();
    void onTextEditorChanged();
    
    //==========================================================================
    // Layout
    //==========================================================================
    
    juce::Rectangle<float> getContentBounds() const;
    juce::Rectangle<float> getSidebarBounds() const;
    SettingsSection* getCurrentSection();
    
    //==========================================================================
    // Persistence
    //==========================================================================
    
    juce::File getSettingsFile() const;
    void loadFromDisk();
    void saveToDisk();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalSettingsPanel)
};

} // namespace zenith
