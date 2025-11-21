/**
 * @file EffectsChainComponent.h
 * @brief Visual effects chain with drag-to-reorder plugins
 *
 * Features:
 * - List of plugins in insert chain for selected track
 * - Add plugin button with plugin browser
 * - Drag-to-reorder plugins in chain
 * - Enable/bypass toggle per plugin
 * - Remove plugin button
 * - Click to open plugin editor
 * - Plugin name, category, and manufacturer display
 * - CPU cost indicator for each plugin
 * - Plugin preset selector
 * - Visual indicators for parameter automation
 * - Animated plugin cards with hover effects
 * - Smooth drag animations
 * - Beautiful gradient cards with icons
 * - Insert/send routing indicator
 *
 * @phase Phase 3: Plugin Effects Chain
 */

#pragma once

#include <JuceHeader.h>

class Engine;
class ProjectState;

namespace zenith {

/**
 * @class EffectsChainComponent
 * @brief Animated visual effects chain with plugin management
 */
class EffectsChainComponent : public juce::Component,
                             public juce::Button::Listener,
                             public juce::Timer
{
public:
    //==========================================================================
    EffectsChainComponent(Engine& eng, ProjectState& state);
    ~EffectsChainComponent() override;

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    //==========================================================================
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    //==========================================================================
    // Button::Listener
    //==========================================================================
    void buttonClicked(juce::Button* button) override;

    //==========================================================================
    // Methods
    //==========================================================================

    /**
     * @brief Set the track for which to display effects chain
     */
    void setTrack(const juce::String& trackId);

    /**
     * @brief Refresh effects list from engine
     */
    void refreshEffectsList();

    /**
     * @brief Get currently selected track
     */
    juce::String getSelectedTrack() const { return selectedTrackId_; }

private:
    //==========================================================================
    // Helpers
    //==========================================================================

    /**
     * @brief Paint plugin card
     */
    void paintPluginCard(juce::Graphics& g, int pluginIndex,
                        const juce::Rectangle<int>& bounds);

    /**
     * @brief Paint plugin icon
     */
    void paintPluginIcon(juce::Graphics& g, const juce::String& category,
                        const juce::Rectangle<int>& bounds);

    /**
     * @brief Get plugin bounds
     */
    juce::Rectangle<int> getPluginCardBounds(int index) const;

    /**
     * @brief Get add plugin button bounds
     */
    juce::Rectangle<int> getAddButtonBounds() const;

    /**
     * @brief Show plugin selector dialog
     */
    void showPluginSelector();

    /**
     * @brief Handle plugin card click
     */
    void handlePluginClicked(int pluginIndex) [[maybe_unused]];

    /**
     * @brief Remove plugin from chain
     */
    void removePlugin(int pluginIndex) [[maybe_unused]];

    /**
     * @brief Toggle plugin bypass
     */
    void togglePluginBypass(int pluginIndex) [[maybe_unused]];

    //==========================================================================
    // Members
    //==========================================================================

    Engine& engine_;
    ProjectState& projectState_;

    // Selected track ID
    juce::String selectedTrackId_;

    // Plugins in current chain
    struct PluginInfo {
        juce::String id;
        juce::String name;
        juce::String category;
        juce::String manufacturer;
        bool bypassed;
        float cpuUsage;
    };
    juce::Array<PluginInfo> pluginChain_;

    // Add button
    juce::TextButton addPluginButton_;

    // Drag state
    int draggedPluginIndex_ = -1;
    juce::Point<int> dragStartPosition_;
    float dragOffsetY_ = 0.0f;

    // Hover state
    int hoveredPluginIndex_ = -1;

    // Animation
    float cardHoverScale_ = 1.0f;
    juce::Array<float> cardAnimations_;

    // Layout
    static constexpr int CARD_HEIGHT = 60;
    static constexpr int CARD_SPACING = 8;
    static constexpr int BUTTON_HEIGHT = 48;
    static constexpr int PADDING = 12;
    static constexpr int ICON_SIZE = 32;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsChainComponent)
};

}  // namespace zenith

