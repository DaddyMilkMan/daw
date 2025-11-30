/**
 * @file PresetBrowserComponent.h
 * @brief Preset browser UI component with save/load/delete functionality
 *
 * Provides a browseable list of presets with:
 * - Search/filter capability
 * - Preset categories
 * - Save/Load/Delete/Rename operations
 * - Integration with ZenithPresetManager
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <string>
#include "../instruments/InstrumentPreset.h"

namespace zenith {

// Forward declaration
class ZenithPresetManager;

class PresetBrowserComponent : public juce::Component,
                                public juce::ListBoxModel
{
public:
    PresetBrowserComponent(const juce::String& instrumentId, 
                          ZenithPresetManager& presetManager)
        : instrumentId_(instrumentId)
        , presetManager_(presetManager)
    {
        // Setup preset list
        addAndMakeVisible(presetList_);
        presetList_.setModel(this);
        presetList_.setRowHeight(24);
        
        // Setup search box
        addAndMakeVisible(searchBox_);
        searchBox_.setTextToShowWhenEmpty("Search presets...", juce::Colours::grey);
        searchBox_.onTextChange = [this] { updateFilteredPresets(); };
        
        // Setup buttons
        addAndMakeVisible(loadButton_);
        loadButton_.setButtonText("Load");
        loadButton_.onClick = [this] { loadSelectedPreset(); };
        
        addAndMakeVisible(saveButton_);
        saveButton_.setButtonText("Save");
        saveButton_.onClick = [this] { showSaveDialog(); };
        
        addAndMakeVisible(deleteButton_);
        deleteButton_.setButtonText("Delete");
        deleteButton_.onClick = [this] { deleteSelectedPreset(); };
        
        refreshPresetList();
    }

    ~PresetBrowserComponent() override = default;

    // Callbacks for loading presets
    std::function<void(const ZenithInstrumentPreset&)> onLoadPreset;
    
    // Callback for capturing current parameter state when saving
    std::function<std::map<std::string, float>()> onCaptureState;

    void setLoadPresetCallback(std::function<void(const ZenithInstrumentPreset&)> callback)
    {
        onLoadPreset = std::move(callback);
    }

    void setCaptureStateCallback(std::function<std::map<std::string, float>()> callback)
    {
        onCaptureState = std::move(callback);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2a2a2a));
        g.setColour(juce::Colours::darkgrey);
        g.drawRect(getLocalBounds(), 1);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4);
        
        // Search box at top
        searchBox_.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(4);
        
        // Buttons at bottom
        auto buttonArea = bounds.removeFromBottom(28);
        auto buttonWidth = buttonArea.getWidth() / 3;
        loadButton_.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
        saveButton_.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
        deleteButton_.setBounds(buttonArea.reduced(2));
        bounds.removeFromBottom(4);
        
        // Preset list in the middle
        presetList_.setBounds(bounds);
    }

    // ListBoxModel implementation
    int getNumRows() override { return filteredPresets_.size(); }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (rowIsSelected)
            g.fillAll(juce::Colour(0xff0066cc));
        else
            g.fillAll(juce::Colour(0xff1a1a1a));

        if (rowNumber < filteredPresets_.size())
        {
            g.setColour(juce::Colours::white);
            g.setFont(14.0f);
            g.drawText(filteredPresets_[rowNumber].name, 4, 0, width - 8, height, 
                      juce::Justification::centredLeft, true);
        }
    }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override
    {
        if (row >= 0 && row < filteredPresets_.size())
            loadPresetAtIndex(row);
    }

private:
    struct PresetInfo
    {
        juce::String name;
        juce::String category;
        juce::File file;
    };

    void refreshPresetList()
    {
        // TODO: Get actual presets from PresetManager
        // For now, just clear
        allPresets_.clear();
        updateFilteredPresets();
    }

    void updateFilteredPresets()
    {
        filteredPresets_.clear();
        auto searchText = searchBox_.getText().toLowerCase();

        for (const auto& preset : allPresets_)
        {
            if (searchText.isEmpty() || 
                preset.name.toLowerCase().contains(searchText) ||
                preset.category.toLowerCase().contains(searchText))
            {
                filteredPresets_.add(preset);
            }
        }

        presetList_.updateContent();
        presetList_.repaint();
    }

    void loadSelectedPreset()
    {
        int selectedRow = presetList_.getSelectedRow();
        if (selectedRow >= 0 && selectedRow < filteredPresets_.size())
            loadPresetAtIndex(selectedRow);
    }

    void loadPresetAtIndex(int index)
    {
        if (onLoadPreset && index >= 0 && index < filteredPresets_.size())
        {
            // TODO: Load actual preset from file
            // For now, just call the callback with a dummy preset
            ZenithInstrumentPreset preset;
            preset.name = filteredPresets_[index].name.toStdString();
            onLoadPreset(preset);
        }
    }

    void showSaveDialog()
    {
        // TODO: Show save dialog
        // For now, just show alert
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                               "Save Preset",
                                               "Save preset functionality coming soon!");
    }

    void deleteSelectedPreset()
    {
        int selectedRow = presetList_.getSelectedRow();
        if (selectedRow >= 0 && selectedRow < filteredPresets_.size())
        {
            // TODO: Delete preset file and refresh list
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                   "Delete Preset",
                                                   "Delete preset functionality coming soon!");
        }
    }

    juce::String instrumentId_;
    ZenithPresetManager& presetManager_;

    juce::ListBox presetList_;
    juce::TextEditor searchBox_;
    juce::TextButton loadButton_;
    juce::TextButton saveButton_;
    juce::TextButton deleteButton_;

    juce::Array<PresetInfo> allPresets_;
    juce::Array<PresetInfo> filteredPresets_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserComponent)
};

} // namespace zenith
