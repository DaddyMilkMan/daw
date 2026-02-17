#include "PresetBrowserComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
#include <cmath>

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

void PresetBrowserComponent::drawSkia(SkCanvas *canvas) {
  SkPaint bg;
  bg.setColor(zenith::design::unified::bg_01());
  canvas->drawRect(SkRect::MakeWH((float)getWidth(), (float)getHeight()), bg);

  SkPaint border;
  border.setStyle(SkPaint::kStroke_Style);
  border.setStrokeWidth(1.0f);
  border.setColor(zenith::design::unified::border_subtle());
  canvas->drawRect(SkRect::MakeWH((float)getWidth(), (float)getHeight()), border);
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

    if (savePresetDialog_) {
      removeChildComponent(savePresetDialog_.get());
      savePresetDialog_.reset();
    }

    savePresetDialog_ = std::make_unique<zenith::SkiaAlertWindow>(
        "Save Preset", "Enter a name for your preset:",
        zenith::SkiaAlertWindow::IconType::QuestionIcon);
    savePresetDialog_->addTextEditor("presetName", preset.name, "Preset Name:");
    savePresetDialog_->addButton("Cancel",
                                 zenith::SkiaAlertWindow::Result::Cancelled,
                                 zenith::SkiaButton::Style::Secondary);
    savePresetDialog_->addButton("Save", zenith::SkiaAlertWindow::Result::Button1,
                                 zenith::SkiaButton::Style::Primary);

    const int w = juce::jlimit(360, 640, (int)std::round((double)getWidth() * 0.82));
    const int h = juce::jlimit(220, 360, (int)std::round((double)getHeight() * 0.70));
    savePresetDialog_->setBounds((getWidth() - w) / 2, (getHeight() - h) / 2, w,
                                 h);
    addAndMakeVisible(savePresetDialog_.get());
    savePresetDialog_->toFront(true);

    savePresetDialog_->showAsync([this, preset](zenith::SkiaAlertWindow::Result result) mutable {
      if (result == zenith::SkiaAlertWindow::Result::Button1 &&
          savePresetDialog_) {
        preset.name = savePresetDialog_->getTextEditorContents("presetName").trim();
        if (preset.name.isNotEmpty()) {
          zenith::ZenithPresetManager::getInstance().savePreset(preset, true);
          refreshPresets();
        }
      }
      if (savePresetDialog_) {
        removeChildComponent(savePresetDialog_.get());
        savePresetDialog_.reset();
      }
    });
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
