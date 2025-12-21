#include "PresetBrowserComponent.h"
#include "../design-system/ZenithTheme.h"

using namespace zenith;

PresetBrowserComponent::PresetBrowserComponent() {
    addAndMakeVisible(presetList);
    presetList.setModel(this);
    presetList.setColour(juce::ListBox::backgroundColourId, ZenithTheme::Colors::bg_02);
    presetList.setRowHeight(30);

    addAndMakeVisible(loadButton);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(deleteButton);
    addAndMakeVisible(refreshButton);

    loadButton.onClick = [this] { loadSelectedPreset(); };
    saveButton.onClick = [this] { saveCurrentPreset(); };
    deleteButton.onClick = [this] { deleteSelectedPreset(); };
    refreshButton.onClick = [this] { refreshPresets(); };

    // Initial refresh
    // Note: In a real app, we might want to delay this or do it async
    // refreshPresets(); 
}

PresetBrowserComponent::~PresetBrowserComponent() {}

void PresetBrowserComponent::paint(juce::Graphics& g) {
    g.fillAll(ZenithTheme::Colors::bg_02);
    
    // Draw a border with design system color
    g.setColour(ZenithTheme::Colors::border_subtle);
    g.drawRect(getLocalBounds(), 1);
}

void PresetBrowserComponent::resized() {
    auto area = getLocalBounds().reduced(10);
    auto buttonArea = area.removeFromBottom(40);
    
    int buttonWidth = buttonArea.getWidth() / 4;
    loadButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    saveButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    deleteButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    refreshButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    
    area.removeFromBottom(10);
    presetList.setBounds(area);
}

int PresetBrowserComponent::getNumRows() {
    return static_cast<int>(presets.size());
}

void PresetBrowserComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) {
    if (rowNumber >= static_cast<int>(presets.size())) return;

    if (rowIsSelected) {
        g.fillAll(ZenithTheme::Colors::accent_subtle);
    }

    g.setColour(ZenithTheme::Colors::text_primary);
    g.setFont(14.0f);
    g.drawText(presets[rowNumber].name, 5, 0, width - 10, height, juce::Justification::centredLeft, true);
    
    g.setColour(ZenithTheme::Colors::text_secondary);
    g.setFont(12.0f);
    g.drawText(presets[rowNumber].category, width - 100, 0, 90, height, juce::Justification::centredRight, true);
}

void PresetBrowserComponent::selectedRowsChanged(int lastRowSelected) {
    // Optional: preview?
}

void PresetBrowserComponent::listBoxItemClicked(int row, const juce::MouseEvent& e) {
    if (e.getNumberOfClicks() == 2) {
        loadSelectedPreset();
    }
}

void PresetBrowserComponent::refreshPresets() {
    presets = zenith::ZenithPresetManager::getInstance().getPresetList(currentInstrumentId);
    presetList.updateContent();
    repaint();
}

void PresetBrowserComponent::setInstrumentId(const juce::String& instrumentId) {
    currentInstrumentId = instrumentId;
    refreshPresets();
}

void PresetBrowserComponent::loadSelectedPreset() {
    int row = presetList.getSelectedRow();
    if (row >= 0 && row < static_cast<int>(presets.size())) {
        auto preset = zenith::ZenithPresetManager::getInstance().loadPreset(currentInstrumentId, presets[row].id);
        if (loadCallback) {
            loadCallback(preset);
        }
        DBG("Loaded preset: " + preset.name);
    }
}

void PresetBrowserComponent::saveCurrentPreset() {
    if (captureCallback) {
        auto preset = captureCallback();
        
        // Create dialog for text input
        auto* window = new juce::AlertWindow("Save Preset", "Enter a name for your preset:", juce::AlertWindow::QuestionIcon, this);
        window->addTextEditor("presetName", preset.name, "Preset Name:");
        window->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
        window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
        
        window->enterModalState(true, juce::ModalCallbackFunction::create([this, window, preset](int result) mutable {
            if (result == 1) {
                preset.name = window->getTextEditorContents("presetName");
                zenith::ZenithPresetManager::getInstance().savePreset(preset, true);
                refreshPresets();
            }
            delete window;
        }));
    }
}

void PresetBrowserComponent::deleteSelectedPreset() {
    int row = presetList.getSelectedRow();
    if (row >= 0 && row < static_cast<int>(presets.size())) {
        zenith::ZenithPresetManager::getInstance().deletePreset(currentInstrumentId, presets[row].id, true); // User preset
        refreshPresets();
    }
}
