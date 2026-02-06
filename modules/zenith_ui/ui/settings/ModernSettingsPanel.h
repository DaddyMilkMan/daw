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
#include "../controls/ZenithButton.h"
#include "../controls/SkiaTextInput.h"
#include "../../Settings.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>
#include <functional>

namespace zenith {

/**
 * Modern, professional settings interface
 * 
 * Design principles:
 * - Card-based layout with proper shadows and spacing
 * - Clear visual hierarchy with typography scale
 * - Contextual help and descriptions for every setting
 * - Smooth animations and hover states
 * - Professional color scheme and iconography
 */
class ModernSettingsPanel : public SkiaComponent {
public:
    ModernSettingsPanel();
    ~ModernSettingsPanel() override;

    // SkiaComponent overrides
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;

    // Settings management
    void refreshFromSettings();
    void applySettings();

    // Animation
    void startShowAnimation();
    void startHideAnimation();

private:
    // Settings categories
    enum class Category {
        Audio,
        Interface,
        Performance,
        Advanced
    };

    // Setting item types
    struct SettingItem {
        juce::String id;
        juce::String title;
        juce::String description;
        juce::String iconPath;
        Category category;
        
        enum Type { Toggle, Dropdown, Slider, Button, Text } type;
        
        // Value storage
        juce::var currentValue;
        juce::var defaultValue;
        juce::StringArray options; // For dropdowns
        float minValue = 0.0f, maxValue = 1.0f; // For sliders
        
        // UI state
        bool isHovered = false;
        bool isActive = false;
        juce::Rectangle<float> bounds;
        
        // Callbacks
        std::function<void(const juce::var&)> onValueChanged;
        std::function<void()> onButtonClicked;
    };

    // Layout constants
    static constexpr int PANEL_WIDTH = 680;
    static constexpr int PANEL_HEIGHT = 520;
    static constexpr int SIDEBAR_WIDTH = 180;
    static constexpr int CARD_PADDING = 24;
    static constexpr int ITEM_HEIGHT = 64;
    static constexpr int SECTION_SPACING = 32;
    static constexpr float CORNER_RADIUS = 12.0f;

    // UI state
    Category activeCategory_ = Category::Audio;
    int hoveredItemIndex_ = -1;
    float animationProgress_ = 0.0f;
    bool isAnimatingIn_ = false;
    
    // Settings data
    std::vector<SettingItem> settings_;
    
    // Components
    std::vector<std::unique_ptr<juce::Component>> dynamicComponents_;
    
    // Methods
    void initializeSettings();
    void setupAudioSettings();
    void setupInterfaceSettings();
    void setupPerformanceSettings();
    void setupAdvancedSettings();
    
    void drawBackground(SkCanvas* canvas);
    void drawSidebar(SkCanvas* canvas);
    void drawMainContent(SkCanvas* canvas);
    void drawSettingCard(SkCanvas* canvas, const SettingItem& item, float y);
    void drawCategoryButton(SkCanvas* canvas, Category category, const juce::String& title, 
                           const juce::String& icon, float y, bool isActive);
    
    juce::Rectangle<float> getMainContentBounds() const;
    juce::Rectangle<float> getSidebarBounds() const;
    std::vector<SettingItem*> getSettingsForCategory(Category category);
    
    void handleCategoryClick(Category category);
    void handleSettingClick(int itemIndex);
    void updateHoverState(const juce::Point<int>& mousePos);
    
    // Animation timer
    std::unique_ptr<juce::Timer> animationTimer_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernSettingsPanel)
};

} // namespace zenith