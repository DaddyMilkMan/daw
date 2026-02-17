#pragma once

#include "../controls/SkiaComboBox.h"
#include "../controls/SkiaLabel.h"
#include "../controls/SkiaListBox.h"
#include "../controls/SkiaTextEditor.h"
#include "../controls/ZenithButton.h"
#include "../framework/SkiaComponent.h"
#include <juce_core/juce_core.h>
#include <unordered_map>

namespace zenith::ui {

class SettingsPanelSkia : public SkiaComponent {
public:
  struct SettingRow {
    juce::String id;
    juce::String name;
    juce::String value;
    juce::String category;
  };

  SettingsPanelSkia();
  ~SettingsPanelSkia() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  void setSettingValue(const juce::String &id, const juce::String &value);
  juce::String getSettingValue(const juce::String &id) const;

private:
  class SettingsModel : public SkiaListBox::Model {
  public:
    explicit SettingsModel(SettingsPanelSkia &owner) : owner_(owner) {}
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, SkCanvas &canvas, int width, int height,
                          bool rowIsSelected) override;

  private:
    SettingsPanelSkia &owner_;
  };

  void filterRows();
  void applySelected();
  void resetSelected();

  std::unique_ptr<SkiaLabel> title_;
  std::unique_ptr<SkiaTextEditor> search_;
  std::unique_ptr<SkiaComboBox> category_;
  std::unique_ptr<SkiaListBox> list_;
  std::unique_ptr<ZenithButton> applyButton_;
  std::unique_ptr<ZenithButton> resetButton_;
  std::unique_ptr<SkiaLabel> status_;

  SettingsModel model_;
  std::vector<SettingRow> rows_;
  std::vector<int> visibleIndexes_;
  std::unordered_map<juce::String, juce::String> values_;
  juce::String searchTerm_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPanelSkia)
};

} // namespace zenith::ui
