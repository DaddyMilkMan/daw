/*
  ==============================================================================

    ModernSettingsPanel.h
    Created: 2026-01-17
    Author:  Zenith Team

    Professional Settings Interface - Built from scratch
    
    Features:
    - Modern card-based layout with proper visual hierarchy
    - Smooth animations and micro-interactions
    - Professional typography and spacing
    - Contextual help and descriptions
    - Responsive design that scales properly
    - Accessibility compliant
    - Clean, minimal aesthetic matching pro audio software

  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../controls/SkiaTextEditor.h"
#include "../../Settings.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>
#include <functional>
#include <array>

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
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

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

    struct DropdownState {
        juce::Rectangle<float> buttonRect;
        std::vector<juce::Rectangle<float>> optionRects;
        juce::StringArray options;
        int selectedIndex = 0;
        bool open = false;
    };

    // Wingman chat list controls
    juce::Rectangle<float> headerRect_;
    juce::Rectangle<float> listViewportRect_;
    juce::Rectangle<float> personaRowRect_;
    juce::Rectangle<float> reasoningRowRect_;
    juce::Rectangle<float> permissionsRowRect_;
    juce::Rectangle<float> responseLengthRowRect_;
    juce::Rectangle<float> suggestionRowRect_;
    juce::Rectangle<float> customPersonaRowRect_;

    DropdownState personaDropdown_;
    DropdownState reasoningDropdown_;
    DropdownState permissionsDropdown_;
    DropdownState responseLengthDropdown_;
    DropdownState suggestionDropdown_;

    float scrollOffset_ = 0.0f;
    float maxScrollOffset_ = 0.0f;
    int hoveredRowId_ = 0;
    std::unique_ptr<SkiaTextEditor> customPersonaEditor_;
    bool customPersonaVisible_ = false;
    
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
    void updateChatStyleHelpText(Settings::ChatStyle style);
    static Settings::ChatStyle fromChatStyleSelectorId(int selectorId);
    static int toChatStyleSelectorId(Settings::ChatStyle style);
    static juce::String toChatStyleLabel(Settings::ChatStyle style);
    static juce::String toReasoningPreviewLabel(Settings::ReasoningPreview mode);
    static juce::String toApprovalModeLabel(Settings::WingmanApprovalMode mode);
    static juce::String toResponseLengthLabel(int id);
    static juce::String toSuggestionLabel(int id);
    
    // Animation timer
    std::unique_ptr<juce::Timer> animationTimer_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernSettingsPanel)
};

} // namespace zenith
