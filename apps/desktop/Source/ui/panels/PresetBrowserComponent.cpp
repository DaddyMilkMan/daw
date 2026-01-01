#include "PresetBrowserComponent.h"

PresetBrowserComponent::PresetBrowserComponent() {
  addAndMakeVisible(presetList);
  presetList.setModel(this);
  // presetList.setColour(juce::ListBox::backgroundColourId,
  // juce::Colour(0xff1e1e1e)); // SkiaListBox uses setColour via SkColor?
  // Checking header it has setColour(SkColor)
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

void PresetBrowserComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff2a2a2a)); // Dark background

  // Draw a border
  g.setColour(juce::Colours::black);
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

void PresetBrowserComponent::paintListBoxItem(int rowNumber, SkCanvas &canvas,
                                              int width, int height,
                                              bool rowIsSelected) {
  if (rowNumber >= static_cast<int>(presets.size()))
    return;

  SkPaint paint;
  if (rowIsSelected) {
    paint.setColor(SkColorSetARGB(51, 0, 255, 255)); // Cyan with alpha ~0.2
  } else {
    paint.setColor(SK_ColorTRANSPARENT);
  }
  canvas.drawRect(SkRect::MakeWH(width, height), paint);

  // Text Name
  SkFont font;
  font.setSize(14.0f);
  paint.setColor(SK_ColorWHITE);

  // Simple draw text (Skia doesn't have easy justification helper without
  // custom logic or SkParagraph/TextBlob, using simple x,y) Centered Left
  // usually means x=5, y=baseline (approx height/2 + size/2)
  canvas.drawString(presets[rowNumber].name.getCharPointer(), 5, height / 2 + 5,
                    font, paint);

  // Category
  font.setSize(12.0f);
  paint.setColor(SkColorSetRGB(128, 128, 128));
  // Right align approx
  float catWidth = font.measureText(
      presets[rowNumber].category.getCharPointer(),
      presets[rowNumber].category.length(), SkTextEncoding::kUTF8);
  canvas.drawString(presets[rowNumber].category.getCharPointer(),
                    width - catWidth - 10, height / 2 + 5, font, paint);
}

// void PresetBrowserComponent::selectedRowsChanged(int lastRowSelected) {} //
// Removed as SkiaListBox doesn't use this override

void PresetBrowserComponent::listBoxItemClicked(int row,
                                                const juce::MouseEvent &e) {
  if (e.getNumberOfClicks() == 2) {
    loadSelectedPreset();
  }
}

void PresetBrowserComponent::refreshPresets() {
  presets = zenith::ZenithPresetManager::getInstance().getPresetList(
      currentInstrumentId);
  presetList.updateContent();
  repaint();
}

void PresetBrowserComponent::setInstrumentId(const juce::String &instrumentId) {
  currentInstrumentId = instrumentId;
  refreshPresets();
}

void PresetBrowserComponent::loadSelectedPreset() {
  int row = presetList.getSelectedRow();
  if (row >= 0 && row < static_cast<int>(presets.size())) {
    auto preset = zenith::ZenithPresetManager::getInstance().loadPreset(
        currentInstrumentId, presets[row].id);
    if (loadCallback) {
      loadCallback(preset);
    }
    DBG("Loaded preset: " + preset.name);
  }
}

void PresetBrowserComponent::saveCurrentPreset() {
  if (captureCallback) {
    auto preset = captureCallback();

    // BUG FIX #13: Implement actual save functionality
    // Use JUCE's simple text input until ZenithDialog is implemented
    auto* asyncBox = new juce::AlertWindow(
        "Save Preset",
        "Enter a name for your preset:",
        juce::MessageBoxIconType::QuestionIcon);
    
    asyncBox->addTextEditor("presetName", preset.name, "Preset Name:");
    asyncBox->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
    asyncBox->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
    
    asyncBox->enterModalState(true, juce::ModalCallbackFunction::create(
        [this, asyncBox, preset](int result) mutable {
          if (result == 1) {
            auto updatedPreset = preset;
            updatedPreset.name = asyncBox->getTextEditorContents("presetName");
            zenith::ZenithPresetManager::getInstance().savePreset(updatedPreset, true);
            refreshPresets();
            DBG("Saved preset: " + updatedPreset.name);
          }
          delete asyncBox;
        }), true);
  }
}

void PresetBrowserComponent::deleteSelectedPreset() {
  int row = presetList.getSelectedRow();
  if (row >= 0 && row < static_cast<int>(presets.size())) {
    zenith::ZenithPresetManager::getInstance().deletePreset(
        currentInstrumentId, presets[row].id, true); // User preset
    refreshPresets();
  }
}
