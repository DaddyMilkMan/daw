#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../instruments/ZenithPresetManager.h"

class PresetBrowserComponent : public juce::Component,
                               public juce::ListBoxModel {
public:
    PresetBrowserComponent();
    ~PresetBrowserComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ListBoxModel overrides
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;

    void refreshPresets();
    void setInstrumentId(const juce::String& instrumentId);

    using LoadCallback = std::function<void(const zenith::Preset&)>;
    using CaptureCallback = std::function<zenith::Preset()>;

    void setLoadPresetCallback(LoadCallback cb) { loadCallback = cb; }
    void setCaptureStateCallback(CaptureCallback cb) { captureCallback = cb; }

private:
    LoadCallback loadCallback;
    CaptureCallback captureCallback;
    juce::ListBox presetList;
    std::vector<zenith::PresetMetadata> presets;
    juce::String currentInstrumentId = "ZenithPolySynth"; // Default

    juce::TextButton loadButton{"Load"};
    juce::TextButton saveButton{"Save"};
    juce::TextButton deleteButton{"Delete"};
    juce::TextButton refreshButton{"Refresh"};

    void loadSelectedPreset();
    void saveCurrentPreset();
    void deleteSelectedPreset();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserComponent)
};
