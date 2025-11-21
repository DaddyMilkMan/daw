/**
 * @file PresetBrowserComponent.cpp
 * @brief Implementation of PresetBrowserComponent
 */

#include "PresetBrowserComponent.h"
#include "AudioFeedback.h"
#include <algorithm>

namespace zenith {

//==============================================================================
// PresetBrowserComponent
//==============================================================================

PresetBrowserComponent::PresetBrowserComponent(
    const juce::String& instrumentId,
    ZenithPresetManager& presetManager)
    : instrumentId_(instrumentId)
    , presetManager_(presetManager)
{
    // Start 60Hz animation timer
    startTimerHz(60);

    // Search field with Apple styling
    addAndMakeVisible(searchLabel_);
    searchLabel_.setText("Search:", juce::dontSendNotification);
    searchLabel_.setJustificationType(juce::Justification::centredLeft);
    searchLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));

    addAndMakeVisible(searchField_);
    searchField_.setTextToShowWhenEmpty("🔍  Search presets...", juce::Colour(0xff606060));
    searchField_.setFont(juce::FontOptions(14.0f));
    searchField_.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2C2C2E));
    searchField_.setColour(juce::TextEditor::textColourId, juce::Colours::white.withAlpha(0.95f));
    searchField_.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff3A3A3C));
    searchField_.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff0A84FF));
    searchField_.onTextChange = [this] { applyFilters(); };

    // Category filter with enhanced styling
    addAndMakeVisible(categoryLabel_);
    categoryLabel_.setText("Category:", juce::dontSendNotification);
    categoryLabel_.setJustificationType(juce::Justification::centredLeft);
    categoryLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));

    addAndMakeVisible(categoryComboBox_);
    categoryComboBox_.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2C2C2E));
    categoryComboBox_.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.95f));
    categoryComboBox_.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3A3A3C));
    categoryComboBox_.setColour(juce::ComboBox::buttonColourId, juce::Colour(0xff0A84FF));
    categoryComboBox_.setColour(juce::ComboBox::arrowColourId, juce::Colours::white.withAlpha(0.8f));
    categoryComboBox_.onChange = [this] { applyFilters(); };

    // Tag search with Apple styling
    addAndMakeVisible(tagLabel_);
    tagLabel_.setText("Tags:", juce::dontSendNotification);
    tagLabel_.setJustificationType(juce::Justification::centredLeft);
    tagLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));

    addAndMakeVisible(tagSearchField_);
    tagSearchField_.setTextToShowWhenEmpty("Filter by tag...", juce::Colour(0xff606060));
    tagSearchField_.setFont(juce::FontOptions(14.0f));
    tagSearchField_.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2C2C2E));
    tagSearchField_.setColour(juce::TextEditor::textColourId, juce::Colours::white.withAlpha(0.95f));
    tagSearchField_.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff3A3A3C));
    tagSearchField_.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff0A84FF));
    tagSearchField_.onTextChange = [this] { applyFilters(); };

    // Preset list with enhanced colors
    listBoxModel_ = std::make_unique<PresetListBoxModel>(*this);
    presetListBox_.setModel(listBoxModel_.get());
    presetListBox_.setRowHeight(32);
    presetListBox_.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1e1e1e));
    presetListBox_.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff3A3A3C));
    addAndMakeVisible(presetListBox_);

    // Preview/Info area
    addAndMakeVisible(previewLabel_);
    previewLabel_.setText("Preset Info", juce::dontSendNotification);
    previewLabel_.setJustificationType(juce::Justification::centredLeft);
    previewLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
    previewLabel_.setFont(juce::FontOptions(12.0f, juce::Font::bold));

    addAndMakeVisible(previewTextEditor_);
    previewTextEditor_.setMultiLine(true);
    previewTextEditor_.setReadOnly(true);
    previewTextEditor_.setFont(juce::FontOptions(12.0f));
    previewTextEditor_.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2C2C2E));
    previewTextEditor_.setColour(juce::TextEditor::textColourId, juce::Colours::white.withAlpha(0.8f));
    previewTextEditor_.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff3A3A3C));
    previewTextEditor_.setText("Select a preset to view details...");

#ifdef ZENITH_USE_SKIA
    // GPU-accelerated action buttons with spring physics
    loadButton_ = std::make_unique<SkiaButtonComponent>("Load", SkiaButtonComponent::Style::Primary);
    loadButton_->onClick = [this] { onPresetSelected(); };
    addAndMakeVisible(*loadButton_);

    saveAsButton_ = std::make_unique<SkiaButtonComponent>("Save As", SkiaButtonComponent::Style::Success);
    saveAsButton_->onClick = [this] { onSaveAsClicked(); };
    addAndMakeVisible(*saveAsButton_);

    initializeButton_ = std::make_unique<SkiaButtonComponent>("Initialize", SkiaButtonComponent::Style::Secondary);
    initializeButton_->onClick = [this] {
        // Initialize to default state
        if (onCaptureState_) {
            statusLabel_.setText("Initialized to default", juce::dontSendNotification);
            statusLabelAlpha_ = 255;
            statusHoldTicks_ = 0;
            statusLabel_.setVisible(true);
        }
    };
    addAndMakeVisible(*initializeButton_);
#else
    // Fallback: Traditional JUCE buttons with Apple colors
    addAndMakeVisible(loadButton_);
    loadButton_.setButtonText("Load");
    loadButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0A84FF));
    loadButton_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    loadButton_.onClick = [this] { onPresetSelected(); };

    addAndMakeVisible(saveAsButton_);
    saveAsButton_.setButtonText("Save As");
    saveAsButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff34C759));
    saveAsButton_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    saveAsButton_.onClick = [this] { onSaveAsClicked(); };

    addAndMakeVisible(initializeButton_);
    initializeButton_.setButtonText("Initialize");
    initializeButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3A3A3C));
    initializeButton_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    initializeButton_.onClick = [this] {
        // Initialize to default state
        if (onCaptureState_) {
            statusLabel_.setText("Initialized to default", juce::dontSendNotification);
            statusLabelAlpha_ = 255;
            statusHoldTicks_ = 0;
            statusLabel_.setVisible(true);
        }
    };
#endif

    // Status with Apple styling
    addAndMakeVisible(statusLabel_);
    statusLabel_.setJustificationType(juce::Justification::centred);
    statusLabel_.setFont(juce::FontOptions(11.0f));
    statusLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    statusLabel_.setColour(juce::Label::backgroundColourId, juce::Colour(0xff2C2C2E));
    statusLabel_.setText("Ready", juce::dontSendNotification);
    statusLabel_.setVisible(false);

    // Load presets
    refreshPresetList();
}

//==============================================================================
void PresetBrowserComponent::refreshPresetList()
{
    // Load all presets for this instrument
    allPresets_ = presetManager_.getPresetsForInstrument(instrumentId_.toStdString());

    // Populate category dropdown
    populateCategories();

    // Apply filters
    applyFilters();

    // Update status
    statusLabel_.setText(
        juce::String(allPresets_.size()) + " presets available",
        juce::dontSendNotification);
}

void PresetBrowserComponent::setCurrentPreset(const std::string& presetId)
{
    currentPresetId_ = presetId;

    // Find and select in list
    for (size_t i = 0; i < filteredPresets_.size(); ++i)
    {
        if (filteredPresets_[i].id == presetId)
        {
            presetListBox_.selectRow((int)i);
            break;
        }
    }
}

void PresetBrowserComponent::timerCallback()
{
    // Smooth focus animations (60Hz, 0.15 speed)
    bool needsRepaint = false;

    // Search field focus animation
    float searchTarget = searchFieldHasFocus_ ? 1.0f : 0.0f;
    if (std::abs(searchFieldFocusAnim_ - searchTarget) > 0.01f)
    {
        searchFieldFocusAnim_ += (searchTarget - searchFieldFocusAnim_) * 0.15f;
        needsRepaint = true;
    }

    // Tag field focus animation
    float tagTarget = tagFieldHasFocus_ ? 1.0f : 0.0f;
    if (std::abs(tagFieldFocusAnim_ - tagTarget) > 0.01f)
    {
        tagFieldFocusAnim_ += (tagTarget - tagFieldFocusAnim_) * 0.15f;
        needsRepaint = true;
    }

    // Status label fade animation
    if (statusLabelAlpha_ > 0)
    {
        statusHoldTicks_++;
        if (statusHoldTicks_ > 120) // Hold for 2 seconds at 60Hz
        {
            statusLabelAlpha_ -= 5;
            if (statusLabelAlpha_ < 0) statusLabelAlpha_ = 0;

            statusLabel_.setColour(juce::Label::textColourId,
                juce::Colours::white.withAlpha(statusLabelAlpha_ / 255.0f));
            needsRepaint = true;
        }
    }

    if (needsRepaint)
        repaint();
}

//==============================================================================
void PresetBrowserComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Minimal gradient
    juce::ColourGradient gradient(
        juce::Colour(0xff242424), 0.0f, 0.0f,
        juce::Colour(0xff1f1f1f), 0.0f, static_cast<float>(bounds.getHeight()),
        false
    );
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds.toFloat(), 12.0f);

    // Soft shadow instead of border
    juce::Path shadowPath;
    shadowPath.addRoundedRectangle(bounds.toFloat(), 12.0f);
    juce::DropShadow shadow(juce::Colours::black.withAlpha(0.3f), 8, juce::Point<int>(0, 2));
    shadow.drawForPath(g, shadowPath);

    // Title with shadow
    g.setColour(juce::Colour(0x00000000).withAlpha(0.3f));
    g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    g.drawText("Preset Browser", 0, 9, getWidth(), 25, juce::Justification::centred);

    g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    g.drawText("Preset Browser", 0, 8, getWidth(), 25, juce::Justification::centred);

    // Subtle separator
    g.setColour(juce::Colour(0xff3a3a3a).withAlpha(0.5f));
    g.drawLine(10.0f, 35.0f, static_cast<float>(getWidth()) - 10.0f, 35.0f, 1.0f);

    // Draw focus glows
    if (searchFieldFocusAnim_ > 0.01f)
    {
        auto fieldBounds = searchField_.getBounds().toFloat().expanded(2.0f);
        g.setColour(juce::Colour(0xff0A84FF).withAlpha(searchFieldFocusAnim_ * 0.3f));
        g.drawRoundedRectangle(fieldBounds, 4.0f, 2.0f);
    }

    if (tagFieldFocusAnim_ > 0.01f)
    {
        auto fieldBounds = tagSearchField_.getBounds().toFloat().expanded(2.0f);
        g.setColour(juce::Colour(0xff0A84FF).withAlpha(tagFieldFocusAnim_ * 0.3f));
        g.drawRoundedRectangle(fieldBounds, 4.0f, 2.0f);
    }
}

void PresetBrowserComponent::resized()
{
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

    // Split remaining space: 60% list, 40% preview
    auto previewHeight = 120;
    auto buttonHeight = 30;
    auto statusHeight = 25;
    auto listHeight = bounds.getHeight() - previewHeight - buttonHeight - statusHeight - 20;

    // Preset list
    presetListBox_.setBounds(bounds.removeFromTop(listHeight));

    bounds.removeFromTop(10);

    // Preview area
    auto previewArea = bounds.removeFromTop(previewHeight);
    previewLabel_.setBounds(previewArea.removeFromTop(20));
    previewTextEditor_.setBounds(previewArea);

    bounds.removeFromTop(5);

    // Three action buttons
    auto buttonRow = bounds.removeFromTop(buttonHeight);
    auto buttonWidth = (buttonRow.getWidth() - 8) / 3;

#ifdef ZENITH_USE_SKIA
    loadButton_->setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(4);
    saveAsButton_->setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(4);
    initializeButton_->setBounds(buttonRow);
#else
    loadButton_.setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(4);
    saveAsButton_.setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(4);
    initializeButton_.setBounds(buttonRow);
#endif

    bounds.removeFromTop(5);

    // Status
    statusLabel_.setBounds(bounds.removeFromTop(statusHeight));
}

//==============================================================================
void PresetBrowserComponent::applyFilters()
{
    filteredPresets_.clear();

    juce::String searchText = searchField_.getText().toLowerCase();
    juce::String tagFilter = tagSearchField_.getText().toLowerCase();
    int selectedCategory = categoryComboBox_.getSelectedId();

    for (const auto& preset : allPresets_)
    {
        if (matchesFilters(preset))
        {
            filteredPresets_.push_back(preset);
        }
    }

    presetListBox_.updateContent();
    presetListBox_.repaint();

    statusLabel_.setText(
        juce::String(filteredPresets_.size()) + " of " +
        juce::String(allPresets_.size()) + " presets",
        juce::dontSendNotification);
}

bool PresetBrowserComponent::matchesFilters(const ZenithInstrumentPreset& preset) const
{
    juce::String searchText = searchField_.getText().toLowerCase();
    juce::String tagFilter = tagSearchField_.getText().toLowerCase();

        // Name search
        if (searchText.isNotEmpty())
        {
            juce::String presetName = juce::String(preset.name).toLowerCase();
            if (!presetName.contains(searchText))
                return false;
        }

        // Tag search
        if (tagFilter.isNotEmpty())
        {
            bool hasMatchingTag = false;
            for (const auto& tag : preset.tags)
            {
                if (juce::String(tag).toLowerCase().contains(tagFilter))
                {
                    hasMatchingTag = true;
                    break;
                }
            }
            if (!hasMatchingTag)
                return false;
        }

        // Category (Sound Type) filter (ID 1 = "All")
        int selectedCategory = categoryComboBox_.getSelectedId();
        if (selectedCategory > 1)
        {
            juce::String categoryName = categoryComboBox_.getItemText(selectedCategory - 1);
            // Use preset.category (or soundType) for matching
            juce::String presetCategory = juce::String(preset.category).toLowerCase();
            if (presetCategory != categoryName.toLowerCase())
                return false;
        }

        // Engine filter (optional future UI) – placeholder logic (always true)
        // Character filter – placeholder (always true)
        return true;
}

void PresetBrowserComponent::populateCategories()
{
    categoryComboBox_.clear();

    categoryComboBox_.addItem("All", 1);

    // Collect unique categories (using author as category for now)
    std::set<std::string> categories;
    for (const auto& preset : allPresets_)
    {
        if (!preset.author.empty())
            categories.insert(preset.author);
    }

    int id = 2;
    for (const auto& category : categories)
    {
        categoryComboBox_.addItem(category, id++);
    }

    categoryComboBox_.setSelectedId(1, juce::dontSendNotification);
}

void PresetBrowserComponent::onPresetSelected()
{
    int selectedRow = presetListBox_.getSelectedRow();
    if (selectedRow >= 0 && selectedRow < (int)filteredPresets_.size())
    {
        const auto& preset = filteredPresets_[selectedRow];
        currentPresetId_ = preset.id;

        // Update preview area with preset details
        juce::String previewText;
        previewText << "Name: " << preset.name << "\n";
        previewText << "Author: " << preset.author << "\n";

        if (!preset.description.empty())
            previewText << "Description: " << preset.description << "\n";

        if (!preset.tags.empty())
        {
            previewText << "Tags: ";
            for (size_t i = 0; i < preset.tags.size(); ++i)
            {
                if (i > 0) previewText << ", ";
                previewText << preset.tags[i];
            }
            previewText << "\n";
        }

        previewText << "\nParameters: " << juce::String(preset.parameters.size());

        previewTextEditor_.setText(previewText);

        // Load the preset if callback is set
        if (onLoadPreset_)
        {
            onLoadPreset_(preset);

            // Play pleasant success chime
            AudioFeedback::getInstance().playSound(AudioFeedback::Success, 0.3f);
        }

        // Show status with fade animation
        statusLabel_.setText(
            "Loaded: " + juce::String(preset.name),
            juce::dontSendNotification);
        statusLabelAlpha_ = 255;
        statusHoldTicks_ = 0;
        statusLabel_.setVisible(true);
    }
}

void PresetBrowserComponent::onSaveAsClicked()
{
    if (!onCaptureState_)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Cannot Save",
            "No state capture callback is configured.",
            "OK");
        return;
    }

    // Ask user for preset name
    juce::AlertWindow nameWindow("Save Preset",
                                "Enter a name for this preset:",
                                juce::AlertWindow::NoIcon);

    nameWindow.addTextEditor("presetName", "My Preset", "Preset Name:");
    nameWindow.addTextEditor("tags", "", "Tags (comma-separated):");
    nameWindow.addTextEditor("description", "", "Description:");

    nameWindow.addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    nameWindow.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    nameWindow.enterModalState(true, juce::ModalCallbackFunction::create([this, &nameWindow](int result) {
        if (result != 1) return;

        juce::String presetName = nameWindow.getTextEditorContents("presetName");
        juce::String tags = nameWindow.getTextEditorContents("tags");
        juce::String description = nameWindow.getTextEditorContents("description");

        if (presetName.isEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Invalid Name",
                "Preset name cannot be empty.",
                "OK");
            return;
        }

        // Capture current state
        auto parameterValues = onCaptureState_();

        // Create preset
        ZenithInstrumentPreset newPreset(
            presetName.toStdString(),
            instrumentId_.toStdString(),
            "User");

        newPreset.description = description.toStdString();

        // Parse tags
        juce::StringArray tagArray = juce::StringArray::fromTokens(tags, ",", "");
        for (const auto& tag : tagArray)
            newPreset.tags.push_back(tag.trim().toStdString());

        // Set parameters
        for (const auto& [paramId, value] : parameterValues)
        {
            newPreset.setParameter(paramId, value);
        }

        // Save to file
        if (presetManager_.saveUserPreset(newPreset))
        {
            statusLabel_.setText("Saved: " + presetName, juce::dontSendNotification);
            refreshPresetList();
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Save Failed",
                "Could not save preset to disk.",
                "OK");
        }
    }), true);
}

juce::String PresetBrowserComponent::getPresetDisplayName(
    const ZenithInstrumentPreset& preset) const
{
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
    PresetBrowserComponent& owner)
    : owner_(owner)
{
}

int PresetBrowserComponent::PresetListBoxModel::getNumRows()
{
    return (int)owner_.filteredPresets_.size();
}

void PresetBrowserComponent::PresetListBoxModel::paintListBoxItem(
    int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= (int)owner_.filteredPresets_.size())
        return;

    const auto& preset = owner_.filteredPresets_[rowNumber];

    // Apple-style selection with gradient and rounded corners
    if (rowIsSelected)
    {
        juce::Rectangle<float> itemBounds(2.0f, 1.0f, (float)width - 4.0f, (float)height - 2.0f);

        // Apple blue gradient for selection
        juce::ColourGradient gradient(
            juce::Colour(0xff0A84FF).brighter(0.1f), 0.0f, 0.0f,
            juce::Colour(0xff0A84FF).darker(0.2f), 0.0f, (float)height,
            false
        );
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(itemBounds, 4.0f);

        // Subtle highlight on top edge
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.fillRoundedRectangle(2.0f, 1.0f, (float)width - 4.0f, 1.0f, 1.0f);
    }
    else
    {
        // Alternating row colors for better readability
        if (rowNumber % 2 == 0)
            g.fillAll(juce::Colour(0xff1e1e1e));
        else
            g.fillAll(juce::Colour(0xff252525));
    }

    // Preset name with enhanced typography
    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(0xffe0e0e0));
    g.setFont(juce::FontOptions(13.0f, juce::Font::plain));

    juce::String displayName = owner_.getPresetDisplayName(preset);

    // Add shadow for selected text
    if (rowIsSelected)
    {
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.drawText(displayName, 9, 1, width - 80, height - 14,
                  juce::Justification::centredLeft, true);
    }

    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(0xffe0e0e0));
    g.drawText(displayName, 8, 0, width - 80, height - 14,
              juce::Justification::centredLeft, true);

    // Tag indicator dots (up to 3 tags shown as colored dots)
    if (!preset.tags.empty())
    {
        int dotX = width - 60;
        int dotY = height / 2 - 3;
        int numDots = std::min(3, (int)preset.tags.size());

        for (int i = 0; i < numDots; ++i)
        {
            g.setColour(rowIsSelected ?
                juce::Colour(0xffffffff).withAlpha(0.6f) :
                juce::Colour(0xff0A84FF).withAlpha(0.5f));
            g.fillEllipse((float)dotX + i * 10.0f, (float)dotY, 6.0f, 6.0f);
        }

        // Show "+N" if more than 3 tags
        if (preset.tags.size() > 3)
        {
            g.setFont(juce::FontOptions(9.0f));
            g.setColour(rowIsSelected ?
                juce::Colours::white.withAlpha(0.7f) :
                juce::Colour(0xff909090));
            juce::String moreText = "+" + juce::String((int)preset.tags.size() - 3);
            g.drawText(moreText, dotX + 32, 0, 20, height,
                      juce::Justification::centredLeft, false);
        }
    }

    // Factory/User badge on the right
    if (!preset.author.empty())
    {
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));

        juce::Colour badgeColor = (preset.author == "Factory") ?
            juce::Colour(0xff34C759) : juce::Colour(0xff0A84FF);

        if (!rowIsSelected)
            badgeColor = badgeColor.withAlpha(0.6f);

        g.setColour(badgeColor);

        juce::String badge = (preset.author == "Factory") ? "F" : "U";
        juce::Rectangle<int> badgeBounds(width - 24, (height - 16) / 2, 18, 16);

        g.fillRoundedRectangle(badgeBounds.toFloat(), 3.0f);

        g.setColour(juce::Colours::white);
        g.drawText(badge, badgeBounds, juce::Justification::centred, false);
    }
}

void PresetBrowserComponent::PresetListBoxModel::listBoxItemClicked(
    int row, const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    if (row >= 0 && row < (int)owner_.filteredPresets_.size())
    {
        owner_.onPresetSelected();
    }
}

void PresetBrowserComponent::PresetListBoxModel::returnKeyPressed(int lastRowSelected)
{
    if (lastRowSelected >= 0 && lastRowSelected < (int)owner_.filteredPresets_.size())
    {
        owner_.onPresetSelected();
    }
}

} // namespace zenith

