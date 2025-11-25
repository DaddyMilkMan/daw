/**
 * @file PresetBrowserComponent.cpp
 * @brief Implementation of PresetBrowserComponent
 */

#include "PresetBrowserComponent.h"
#include "ZenithLookAndFeel.h"
#include <algorithm>


namespace zenith {

//==============================================================================
// PresetBrowserComponent
//==============================================================================

PresetBrowserComponent::PresetBrowserComponent(
    const juce::String &instrumentId, ZenithPresetManager &presetManager)
    : instrumentId_(instrumentId), presetManager_(presetManager) {
  // Search field
  addAndMakeVisible(searchLabel_);
  searchLabel_.setText("Search:", juce::dontSendNotification);
  searchLabel_.setJustificationType(juce::Justification::centredLeft);

  addAndMakeVisible(searchField_);
  searchField_.setTextToShowWhenEmpty("Type to search...", juce::Colours::grey);
  searchField_.onTextChange = [this] { applyFilters(); };

  // Category filter
  addAndMakeVisible(categoryLabel_);
  categoryLabel_.setText("Category:", juce::dontSendNotification);
  categoryLabel_.setJustificationType(juce::Justification::centredLeft);

  addAndMakeVisible(categoryComboBox_);
  categoryComboBox_.onChange = [this] { applyFilters(); };

  // Tag search
  addAndMakeVisible(tagLabel_);
  tagLabel_.setText("Tags:", juce::dontSendNotification);
  tagLabel_.setJustificationType(juce::Justification::centredLeft);

  addAndMakeVisible(tagSearchField_);
  tagSearchField_.setTextToShowWhenEmpty("Filter by tag...",
                                         juce::Colours::grey);
  tagSearchField_.onTextChange = [this] { applyFilters(); };

  // Preset list
  listBoxModel_ = std::make_unique<PresetListBoxModel>(*this);
  presetListBox_.setModel(listBoxModel_.get());
  presetListBox_.setRowHeight(24);
  addAndMakeVisible(presetListBox_);

  // Buttons
  addAndMakeVisible(saveAsButton_);
  saveAsButton_.setButtonText("Save As...");
  saveAsButton_.onClick = [this] { onSaveAsClicked(); };

  // Status
  addAndMakeVisible(statusLabel_);
  statusLabel_.setJustificationType(juce::Justification::centred);
  statusLabel_.setText("Ready", juce::dontSendNotification);

  // Load presets
  refreshPresetList();
}

//==============================================================================
void PresetBrowserComponent::refreshPresetList() {
  // Load all presets for this instrument
  allPresets_ =
      presetManager_.getPresetsForInstrument(instrumentId_.toStdString());

  // Populate category dropdown
  populateCategories();

  // Apply filters
  applyFilters();

  // Update status
  statusLabel_.setText(juce::String(allPresets_.size()) + " presets available",
                       juce::dontSendNotification);
}

void PresetBrowserComponent::setCurrentPreset(const std::string &presetId) {
  currentPresetId_ = presetId;

  // Find and select in list
  for (size_t i = 0; i < filteredPresets_.size(); ++i) {
    if (filteredPresets_[i].id == presetId) {
      presetListBox_.selectRow((int)i);
      break;
    }
  }
}

//==============================================================================
void PresetBrowserComponent::paint(juce::Graphics &g) {
  // Modern dark background
  g.fillAll(juce::Colour(ZenithLookAndFeel::Colors::backgroundBase));

  // Title with better typography
  g.setColour(juce::Colours::white);
  g.setFont(juce::Font("Inter", 18.0f, juce::Font::bold));
  g.drawText("Preset Browser", 0, 8, getWidth(), 25,
             juce::Justification::centred);

  // Subtle separator
  g.setColour(juce::Colour(ZenithLookAndFeel::Colors::borderMedium));
  g.drawLine(10.0f, 35.0f, (float)getWidth() - 10.0f, 35.0f, 1.0f);
}

void PresetBrowserComponent::resized() {
  auto bounds = getLocalBounds().reduced(10);
  bounds.removeFromTop(35); // Title space

  // Search controls
  auto searchRow = bounds.removeFromTop(25);
  searchLabel_.setBounds(searchRow.removeFromLeft(60));
  searchField_.setBounds(searchRow);

  bounds.removeFromTop(5);

  // Category filter
  auto categoryRow = bounds.removeFromTop(25);
  categoryLabel_.setBounds(categoryRow.removeFromLeft(60));
  categoryComboBox_.setBounds(categoryRow);

  bounds.removeFromTop(5);

  // Tag filter
  auto tagRow = bounds.removeFromTop(25);
  tagLabel_.setBounds(tagRow.removeFromLeft(60));
  tagSearchField_.setBounds(tagRow);

  bounds.removeFromTop(10);

  // Preset list (takes remaining space)
  auto buttonHeight = 30;
  auto statusHeight = 25;
  auto listHeight = bounds.getHeight() - buttonHeight - statusHeight - 10;
  presetListBox_.setBounds(bounds.removeFromTop(listHeight));

  bounds.removeFromTop(5);

  // Buttons
  auto buttonRow = bounds.removeFromTop(buttonHeight);
  saveAsButton_.setBounds(
      buttonRow.removeFromLeft(buttonRow.getWidth() / 2 - 2));
  // Refresh button removed
  buttonRow.removeFromLeft(4);

  bounds.removeFromTop(5);

  // Status
  statusLabel_.setBounds(bounds.removeFromTop(statusHeight));
}

//==============================================================================
void PresetBrowserComponent::applyFilters() {
  filteredPresets_.clear();

  juce::String searchText = searchField_.getText().toLowerCase();
  juce::String tagFilter = tagSearchField_.getText().toLowerCase();
  int selectedCategory = categoryComboBox_.getSelectedId();

  for (const auto &preset : allPresets_) {
    if (matchesFilters(preset)) {
      filteredPresets_.push_back(preset);
    }
  }

  presetListBox_.updateContent();
  presetListBox_.repaint();

  statusLabel_.setText(juce::String(filteredPresets_.size()) + " of " +
                           juce::String(allPresets_.size()) + " presets",
                       juce::dontSendNotification);
}

bool PresetBrowserComponent::matchesFilters(
    const ZenithInstrumentPreset &preset) const {
  juce::String searchText = searchField_.getText().toLowerCase();
  juce::String tagFilter = tagSearchField_.getText().toLowerCase();

  // Name search
  if (searchText.isNotEmpty()) {
    juce::String presetName = juce::String(preset.name).toLowerCase();
    if (!presetName.contains(searchText))
      return false;
  }

  // Tag search
  if (tagFilter.isNotEmpty()) {
    bool hasMatchingTag = false;
    for (const auto &tag : preset.tags) {
      if (juce::String(tag).toLowerCase().contains(tagFilter)) {
        hasMatchingTag = true;
        break;
      }
    }
    if (!hasMatchingTag)
      return false;
  }

  // Category filter (ID 1 = "All")
  int selectedCategory = categoryComboBox_.getSelectedId();
  if (selectedCategory > 1) {
    juce::String categoryName =
        categoryComboBox_.getItemText(selectedCategory - 1);
    // For now, we'll use author as category (Factory/User)
    if (categoryName != juce::String(preset.author))
      return false;
  }

  return true;
}

void PresetBrowserComponent::populateCategories() {
  categoryComboBox_.clear();

  categoryComboBox_.addItem("All", 1);

  // Collect unique categories (using author as category for now)
  std::set<std::string> categories;
  for (const auto &preset : allPresets_) {
    if (!preset.author.empty())
      categories.insert(preset.author);
  }

  int id = 2;
  for (const auto &category : categories) {
    categoryComboBox_.addItem(category, id++);
  }

  categoryComboBox_.setSelectedId(1, juce::dontSendNotification);
}

void PresetBrowserComponent::onPresetSelected() {
  int selectedRow = presetListBox_.getSelectedRow();
  if (selectedRow >= 0 && selectedRow < (int)filteredPresets_.size()) {
    const auto &preset = filteredPresets_[selectedRow];
    currentPresetId_ = preset.id;

    if (onLoadPreset_) {
      onLoadPreset_(preset);
    }

    statusLabel_.setText("Loaded: " + juce::String(preset.name),
                         juce::dontSendNotification);
  }
}

void PresetBrowserComponent::onSaveAsClicked() {
  if (!onCaptureState_) {
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon, "Cannot Save",
        "No state capture callback is configured.", "OK");
    return;
  }

  // In JUCE 8, use async dialogs instead of runModalLoop
  // For now, just show a placeholder message
  juce::AlertWindow::showMessageBoxAsync(
      juce::AlertWindow::InfoIcon, "Save Preset",
      "Preset saving is temporarily disabled (requires JUCE 8 async dialog "
      "refactor).",
      "OK");

  // TODO: Implement async save dialog using JUCE 8 API
}

juce::String PresetBrowserComponent::getPresetDisplayName(
    const ZenithInstrumentPreset &preset) const {
  juce::String name = preset.name;

  // Add author badge
  if (preset.author == "Factory")
    name += " [F]";
  else if (preset.author == "User")
    name += " [U]";

  return name;
}

//==============================================================================
// PresetListBoxModel
//==============================================================================

PresetBrowserComponent::PresetListBoxModel::PresetListBoxModel(
    PresetBrowserComponent &owner)
    : owner_(owner) {}

int PresetBrowserComponent::PresetListBoxModel::getNumRows() {
  return (int)owner_.filteredPresets_.size();
}

void PresetBrowserComponent::PresetListBoxModel::paintListBoxItem(
    int rowNumber, juce::Graphics &g, int width, int height,
    bool rowIsSelected) {
  if (rowNumber < 0 || rowNumber >= (int)owner_.filteredPresets_.size())
    return;

  const auto &preset = owner_.filteredPresets_[rowNumber];

  // Modern background colors with smooth transitions
  if (rowIsSelected) {
    // Animated gradient for selection
    juce::ColourGradient gradient(
        juce::Colour(ZenithLookAndFeel::Colors::accentPrimary), 0.0f, 0.0f,
        juce::Colour(ZenithLookAndFeel::Colors::accentPrimaryPressed),
        (float)width, 0.0f, false);
    g.setGradientFill(gradient);
    g.fillRect(0, 0, width, height);
  } else if (rowNumber % 2 == 0)
    g.fillAll(juce::Colour(ZenithLookAndFeel::Colors::backgroundRaised));
  else
    g.fillAll(juce::Colour(ZenithLookAndFeel::Colors::backgroundBase));

  // Text with better contrast and smooth fade
  g.setColour(rowIsSelected
                  ? juce::Colours::white
                  : juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
  g.setFont(juce::Font("Inter", 13.0f, juce::Font::plain));

  juce::String displayName = owner_.getPresetDisplayName(preset);
  g.drawText(displayName, 8, 0, width - 16, height,
             juce::Justification::centredLeft, true);

  // Tags (small, subtle) with fade effect
  if (!preset.tags.empty()) {
    g.setFont(juce::Font("Inter", 9.0f, juce::Font::plain));
    g.setColour(rowIsSelected
                    ? juce::Colour(ZenithLookAndFeel::Colors::textPrimary)
                          .withAlpha(0.8f)
                    : juce::Colour(ZenithLookAndFeel::Colors::textSecondary));

    juce::String tagStr;
    for (size_t i = 0; i < preset.tags.size() && i < 3; ++i) {
      if (i > 0)
        tagStr += ", ";
      tagStr += juce::String(preset.tags[i]);
    }
    if (preset.tags.size() > 3)
      tagStr += "...";

    g.drawText(tagStr, 8, height - 14, width - 16, 12,
               juce::Justification::centredRight, true);
  }
}

void PresetBrowserComponent::PresetListBoxModel::listBoxItemClicked(
    int row, const juce::MouseEvent &event) {
  juce::ignoreUnused(event);
  if (row >= 0 && row < (int)owner_.filteredPresets_.size()) {
    owner_.onPresetSelected();
  }
}

void PresetBrowserComponent::PresetListBoxModel::returnKeyPressed(
    int lastRowSelected) {
  if (lastRowSelected >= 0 &&
      lastRowSelected < (int)owner_.filteredPresets_.size()) {
    owner_.onPresetSelected();
  }
}

//==============================================================================
void PresetBrowserComponent::timerCallback() {
  // Timer callback for periodic preset list refresh
  // Currently unused - can be used for dynamic preset monitoring
}

} // namespace zenith
