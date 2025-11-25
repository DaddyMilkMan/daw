/**
 * @file PresetBrowserComponent.h
 * @brief Reusable preset browser for instrument editors
 *
 * Features:
 * - Lists presets for an instrument
 * - Category dropdown filter
 * - Tag search field
 * - Text search by name
 * - Load preset on click
 * - Save As functionality
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include "../instruments/InstrumentPreset.h"
#include "../instruments/Instrument.h"
#include <vector>
#include <functional>

// Note: Skia rendering enabled for other components but buttons use JUCE for now

namespace zenith {

//==============================================================================
/**
 * @brief Preset browser component for instruments
 *
 * This component can be embedded in instrument editors to provide
 * preset browsing and management functionality.
 *
 * Features Apple-inspired design with gradients, rounded corners,
 * smooth animations, and enhanced visual feedback.
 */
class PresetBrowserComponent : public juce::Component,
                               public juce::Timer
{
public:
    //==========================================================================
    // Constructor
    //==========================================================================

    /**
     * @brief Create preset browser for an instrument
     * @param instrumentId The instrument ID (e.g., "zenith_poly_synth")
     * @param presetManager Reference to the preset manager
     */
    PresetBrowserComponent(const juce::String& instrumentId,
                          ZenithPresetManager& presetManager);

    ~PresetBrowserComponent() override = default;

    //==========================================================================
    // Callbacks
    //==========================================================================

    /**
     * @brief Set callback for when user selects a preset
     * @param callback Function to call with the selected preset
     */
    void setLoadPresetCallback(std::function<void(const ZenithInstrumentPreset&)> callback)
    {
        onLoadPreset_ = std::move(callback);
    }

    /**
     * @brief Set callback for when user wants to capture current state
     * @param callback Function that returns current parameter state
     */
    void setCaptureStateCallback(
        std::function<std::map<std::string, float>()> callback)
    {
        onCaptureState_ = std::move(callback);
    }

    //==========================================================================
    // UI Management
    //==========================================================================

    /**
     * @brief Refresh the preset list
     */
    void refreshPresetList();

    /**
     * @brief Set the currently selected preset (updates UI)
     * @param presetId The preset ID to select
     */
    void setCurrentPreset(const std::string& presetId);

    //==========================================================================
    // Component overrides
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    //==========================================================================
    // Internal Methods
    //==========================================================================

    void applyFilters();
    void onPresetSelected();
    void onSaveAsClicked();
    void populateCategories();

    juce::String getPresetDisplayName(const ZenithInstrumentPreset& preset) const;
    bool matchesFilters(const ZenithInstrumentPreset& preset) const;

    //==========================================================================
    // Member Variables
    //==========================================================================

    juce::String instrumentId_;
    ZenithPresetManager& presetManager_;

    // All available presets
    std::vector<ZenithInstrumentPreset> allPresets_;

    // Filtered presets
    std::vector<ZenithInstrumentPreset> filteredPresets_;

    // Current selection
    std::string currentPresetId_;

    // Callbacks
    std::function<void(const ZenithInstrumentPreset&)> onLoadPreset_;
    std::function<std::map<std::string, float>()> onCaptureState_;

    //==========================================================================
    // UI Components
    //==========================================================================

    // Search/Filter controls
    juce::Label searchLabel_;
    juce::TextEditor searchField_;

    juce::Label categoryLabel_;
    juce::ComboBox categoryComboBox_;

    juce::Label tagLabel_;
    juce::TextEditor tagSearchField_;

    // Preset list
    juce::ListBox presetListBox_;

    // Action buttons (JUCE for now)
    juce::TextButton saveAsButton_;
    juce::TextButton initializeButton_;
    juce::TextButton loadButton_;

    // Status
    juce::Label statusLabel_;
    int statusLabelAlpha_ = 0;
    int statusHoldTicks_ = 0;

    // Preview/Info area
    juce::Label previewLabel_;
    juce::TextEditor previewTextEditor_;

    // Animation state
    float searchFieldFocusAnim_ = 0.0f;
    float tagFieldFocusAnim_ = 0.0f;
    bool searchFieldHasFocus_ = false;
    bool tagFieldHasFocus_ = false;

    //==========================================================================
    // ListBoxModel for preset display
    //==========================================================================

    class PresetListBoxModel : public juce::ListBoxModel
    {
    public:
        PresetListBoxModel(PresetBrowserComponent& owner);

        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g,
                            int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent& event) override;
        void returnKeyPressed(int lastRowSelected) [[maybe_unused]] override;

    private:
        PresetBrowserComponent& owner_;
    };

    std::unique_ptr<PresetListBoxModel> listBoxModel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserComponent)
};

} // namespace zenith

