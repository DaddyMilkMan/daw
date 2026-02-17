#include "SettingsPanelSkia.h"
#include "../design-system/ZenithDesignSystem.h"

namespace zenith::ui {

SettingsPanelSkia::SettingsPanelSkia() : model_(*this) {
  title_ = std::make_unique<SkiaLabel>("settings_title", "Settings");
  title_->setFont(18.0f, SkFontStyle::kBold_Weight);
  title_->setTextColour(design::colors::TEXT_PRIMARY);
  addAndMakeVisible(title_.get());

  search_ = std::make_unique<SkiaTextEditor>("settings_search");
  search_->setTextToShowWhenEmpty("Search settings...",
                                  design::withAlpha(design::colors::TEXT_SECONDARY, 0.7f));
  search_->onTextChange = [this]() {
    searchTerm_ = search_->getText().toLowerCase().trim();
    filterRows();
  };
  addAndMakeVisible(search_.get());

  category_ = std::make_unique<SkiaComboBox>("settings_category");
  category_->addItem("All", 1);
  category_->addItem("Audio", 2);
  category_->addItem("Display", 3);
  category_->addItem("MIDI", 4);
  category_->setSelectedId(1);
  category_->onChange = [this]() { filterRows(); };
  addAndMakeVisible(category_.get());

  list_ = std::make_unique<SkiaListBox>("settings_list");
  list_->setModel(&model_);
  list_->setRowHeight(34);
  addAndMakeVisible(list_.get());

  applyButton_ = std::make_unique<ZenithButton>("Apply");
  applyButton_->setStyle(ZenithButton::Style::Primary);
  applyButton_->onClick = [this]() { applySelected(); };
  addAndMakeVisible(applyButton_.get());

  resetButton_ = std::make_unique<ZenithButton>("Reset");
  resetButton_->setStyle(ZenithButton::Style::Secondary);
  resetButton_->onClick = [this]() { resetSelected(); };
  addAndMakeVisible(resetButton_.get());

  status_ = std::make_unique<SkiaLabel>("settings_status", "Ready");
  status_->setTextColour(design::colors::TEXT_SECONDARY);
  addAndMakeVisible(status_.get());

  rows_ = {
      {"buffer_size", "Buffer Size", "512", "Audio"},
      {"sample_rate", "Sample Rate", "48000", "Audio"},
      {"ui_scale", "UI Scale", "100%", "Display"},
      {"theme", "Theme", "Zenith", "Display"},
      {"midi_thru", "MIDI Thru", "On", "MIDI"},
  };
  for (const auto &row : rows_) {
    values_[row.id] = row.value;
  }

  filterRows();
}

SettingsPanelSkia::~SettingsPanelSkia() = default;

void SettingsPanelSkia::drawSkia(SkCanvas *canvas) {
  SkPaint bg;
  bg.setColor(design::colors::BG_01);
  canvas->drawRect(SkRect::MakeWH((float)getWidth(), (float)getHeight()), bg);
}

void SettingsPanelSkia::resized() {
  auto area = getLocalBounds().reduced(10);
  auto header = area.removeFromTop(34);
  title_->setBounds(header.removeFromLeft(180));

  auto filters = area.removeFromTop(32);
  search_->setBounds(filters.removeFromLeft(280));
  filters.removeFromLeft(8);
  category_->setBounds(filters.removeFromLeft(180));

  area.removeFromTop(8);
  auto footer = area.removeFromBottom(36);
  applyButton_->setBounds(footer.removeFromRight(120));
  footer.removeFromRight(8);
  resetButton_->setBounds(footer.removeFromRight(120));
  status_->setBounds(footer);

  list_->setBounds(area);
}

void SettingsPanelSkia::setSettingValue(const juce::String &id,
                                        const juce::String &value) {
  values_[id] = value;
  for (auto &row : rows_) {
    if (row.id == id) {
      row.value = value;
      break;
    }
  }
  list_->updateContent();
}

juce::String SettingsPanelSkia::getSettingValue(const juce::String &id) const {
  auto it = values_.find(id);
  return it == values_.end() ? juce::String() : it->second;
}

int SettingsPanelSkia::SettingsModel::getNumRows() {
  return (int)owner_.visibleIndexes_.size();
}

void SettingsPanelSkia::SettingsModel::paintListBoxItem(int rowNumber,
                                                        SkCanvas &canvas,
                                                        int width, int height,
                                                        bool rowIsSelected) {
  if (rowNumber < 0 || rowNumber >= (int)owner_.visibleIndexes_.size())
    return;
  const auto &row = owner_.rows_[(size_t)owner_.visibleIndexes_[(size_t)rowNumber]];

  if (rowIsSelected) {
    SkPaint sel;
    sel.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.2f));
    canvas.drawRect(SkRect::MakeWH((float)width, (float)height), sel);
  }

  SkFont nameFont = design::getSkFont(12.0f, design::FontWeight::Medium);
  SkPaint namePaint;
  namePaint.setAntiAlias(true);
  namePaint.setColor(design::colors::TEXT_PRIMARY);
  canvas.drawString(row.name.toRawUTF8(), 10.0f, 15.0f, nameFont, namePaint);

  SkFont metaFont = design::getSkFont(10.5f, design::FontWeight::Regular);
  SkPaint metaPaint;
  metaPaint.setAntiAlias(true);
  metaPaint.setColor(design::colors::TEXT_SECONDARY);
  canvas.drawString((row.category + "  " + row.value).toRawUTF8(), 10.0f, 28.0f,
                    metaFont, metaPaint);
}

void SettingsPanelSkia::filterRows() {
  visibleIndexes_.clear();
  const int catId = category_->getSelectedId();

  for (int i = 0; i < (int)rows_.size(); ++i) {
    const auto &r = rows_[(size_t)i];
    const bool searchOk = searchTerm_.isEmpty() ||
                          r.name.toLowerCase().contains(searchTerm_) ||
                          r.id.toLowerCase().contains(searchTerm_);
    bool catOk = true;
    if (catId == 2)
      catOk = (r.category == "Audio");
    else if (catId == 3)
      catOk = (r.category == "Display");
    else if (catId == 4)
      catOk = (r.category == "MIDI");

    if (searchOk && catOk)
      visibleIndexes_.push_back(i);
  }

  list_->updateContent();
  status_->setText("Visible: " + juce::String((int)visibleIndexes_.size()),
                   juce::dontSendNotification);
}

void SettingsPanelSkia::applySelected() {
  status_->setText("Applied", juce::dontSendNotification);
  status_->setTextColour(design::colors::SUCCESS);
}

void SettingsPanelSkia::resetSelected() {
  const int row = list_->getSelectedRow();
  if (row >= 0 && row < (int)visibleIndexes_.size()) {
    auto &target = rows_[(size_t)visibleIndexes_[(size_t)row]];
    setSettingValue(target.id, "Default");
    status_->setText("Reset: " + target.name, juce::dontSendNotification);
  }
}

} // namespace zenith::ui
