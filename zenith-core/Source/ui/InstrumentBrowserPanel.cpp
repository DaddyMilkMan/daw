/**
 * @file InstrumentBrowserPanel.cpp
 * @brief Instrument & Preset Browser UI implementation
 */

#include "InstrumentBrowserPanel.h"
#include "../../include/Engine.h"
#include <algorithm>

namespace zenith {

//==============================================================================
// Tag chip definitions
//==============================================================================

static const juce::StringArray TAG_CHIPS = {
    "All", "Bass", "Lead", "Pad", "Pluck", "Keys", "FX", "808", "Bell", "Arp"
};

//==============================================================================
// InstrumentBrowserPanel Implementation
//==============================================================================

InstrumentBrowserPanel::InstrumentBrowserPanel(Engine& engine, ProjectState& projectState)
    : engine_(engine),
      projectState_(projectState),
      instrumentRegistry_(InstrumentRegistry::getInstance())
{
    // Title header
    titleLabel.setText("Instruments", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    // Search box
    searchLabel.setText("Search:", juce::dontSendNotification);
    searchLabel.setFont(juce::Font(14.0f));
    searchLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(searchLabel);

    searchBox.setMultiLine(false);
    searchBox.setReturnKeyStartsNewLine(false);
    searchBox.setTextToShowWhenEmpty("Search presets...", juce::Colours::grey);
    searchBox.setFont(juce::Font(14.0f));
    searchBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2a2a2a));
    searchBox.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    searchBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff3a3a3a));
    searchBox.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff4a9eff));
    searchBox.addListener(this);
    addAndMakeVisible(searchBox);

    // Tag chips
    tagsLabel.setText("Tags:", juce::dontSendNotification);
    tagsLabel.setFont(juce::Font(14.0f));
    tagsLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(tagsLabel);

    addAndMakeVisible(tagChipsContainer);
    initializeTagChips();

    // Instrument list
    instrumentsLabel.setText("Instruments:", juce::dontSendNotification);
    instrumentsLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    instrumentsLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(instrumentsLabel);

    instrumentListModel_ = std::make_unique<InstrumentListBoxModel>(*this);
    instrumentList.setModel(instrumentListModel_.get());
    instrumentList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    instrumentList.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff3a3a3a));
    instrumentList.setRowHeight(30);
    addAndMakeVisible(instrumentList);

    // Preset list
    presetsLabel.setText("Presets:", juce::dontSendNotification);
    presetsLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    presetsLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(presetsLabel);

    presetListModel_ = std::make_unique<PresetListBoxModel>(*this);
    presetList.setModel(presetListModel_.get());
    presetList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    presetList.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff3a3a3a));
    presetList.setRowHeight(28);
    addAndMakeVisible(presetList);

    // Load preset button
    loadPresetButton.setButtonText("Load to Track");
    loadPresetButton.setEnabled(false);
    loadPresetButton.onClick = [this]() { loadPresetToSelectedTrack(); };
    addAndMakeVisible(loadPresetButton);

    // Status label (toast)
    statusLabel.setFont(juce::Font(12.0f));
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff2a2a2a));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    statusLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff4a9eff));
    statusLabel.setVisible(false);
    addAndMakeVisible(statusLabel);

    // Load instruments
    instrumentIds_ = instrumentRegistry_.getInstrumentIds();
    instrumentList.updateContent();

    setSize(300, 600);
}

InstrumentBrowserPanel::~InstrumentBrowserPanel()
{
    searchBox.removeListener(this);
}

void InstrumentBrowserPanel::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff252525));

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(getLocalBounds(), 1);
}

void InstrumentBrowserPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Title
    titleLabel.setBounds(bounds.removeFromTop(30));

    bounds.removeFromTop(10);  // Spacing

    // Search section
    searchLabel.setBounds(bounds.removeFromTop(20));
    searchBox.setBounds(bounds.removeFromTop(30));

    bounds.removeFromTop(10);  // Spacing

    // Tag chips section
    tagsLabel.setBounds(bounds.removeFromTop(20));
    auto tagChipsArea = bounds.removeFromTop(80);  // 2 rows of chips
    tagChipsContainer.setBounds(tagChipsArea);

    // Layout tag chips in a grid (5 per row)
    int chipWidth = 55;
    int chipHeight = 28;
    int chipSpacing = 5;
    int x = 0;
    int y = 0;

    for (int i = 0; i < tagChips.size(); ++i)
    {
        tagChips[i]->setBounds(x, y, chipWidth, chipHeight);

        x += chipWidth + chipSpacing;
        if ((i + 1) % 5 == 0)  // New row every 5 chips
        {
            x = 0;
            y += chipHeight + chipSpacing;
        }
    }

    bounds.removeFromTop(10);  // Spacing

    // Instrument list section
    instrumentsLabel.setBounds(bounds.removeFromTop(20));
    auto instrumentListArea = bounds.removeFromTop(150);
    instrumentList.setBounds(instrumentListArea);

    bounds.removeFromTop(10);  // Spacing

    // Preset list section
    presetsLabel.setBounds(bounds.removeFromTop(20));

    // Load button at bottom
    auto loadButtonArea = bounds.removeFromBottom(35);
    loadPresetButton.setBounds(loadButtonArea);

    bounds.removeFromBottom(5);  // Spacing

    // Preset list takes remaining space
    presetList.setBounds(bounds);

    // Status label (overlay at bottom)
    statusLabel.setBounds(getLocalBounds().removeFromBottom(40).reduced(15, 10));
}

void InstrumentBrowserPanel::toggleVisibility()
{
    setPanelVisible(!isVisible());
}

void InstrumentBrowserPanel::setPanelVisible(bool shouldBeVisible)
{
    setVisible(shouldBeVisible);
}

//==============================================================================
// TextEditor::Listener
//==============================================================================

void InstrumentBrowserPanel::textEditorTextChanged(juce::TextEditor& editor)
{
    if (&editor == &searchBox)
    {
        updateSearchFilter();
    }
}

//==============================================================================
// Internal methods
//==============================================================================

void InstrumentBrowserPanel::initializeTagChips()
{
    for (const auto& tagName : TAG_CHIPS)
    {
        auto chip = std::make_unique<juce::TextButton>(tagName);
        chip->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
        chip->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff4a9eff));
        chip->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        chip->setColour(juce::TextButton::textColourOnId, juce::Colours::white);

        chip->setClickingTogglesState(true);
        chip->setRadioGroupId(1000);  // Exclusive selection

        chip->onClick = [this, tagName]() {
            updateTagFilter(tagName);
        };

        tagChipsContainer.addAndMakeVisible(chip.get());
        tagChips.push_back(std::move(chip));
    }

    // Select "All" by default
    if (!tagChips.empty())
    {
        tagChips[0]->setToggleState(true, juce::dontSendNotification);
        activeTag = "";  // Empty = show all
    }
}

void InstrumentBrowserPanel::onInstrumentSelected(int instrumentIndex)
{
    if (instrumentIndex < 0 || instrumentIndex >= instrumentIds_.size())
        return;

    selectedInstrumentId_ = instrumentIds_[instrumentIndex];

    // Load presets for this instrument (on background thread to avoid UI freeze)
    juce::MessageManager::callAsync([this, instrumentId = selectedInstrumentId_]() {
        // Load presets
        auto presets = presetManager_.getPresetsForInstrument(instrumentId.toStdString());

        // Convert to PresetItem vector
        presetItems_.clear();
        for (const auto& preset : presets)
        {
            PresetItem item;
            item.preset = preset;
            item.matchesSearch = true;
            item.matchesTag = true;
            presetItems_.push_back(item);
        }

        // Apply current filters
        applyFilters();

        // Update preset list
        presetList.updateContent();
        presetList.repaint();

        // Update status
        showStatus("Loaded " + juce::String(presets.size()) + " presets for " +
                   juce::String(instrumentId));
    });
}

void InstrumentBrowserPanel::onPresetDoubleClicked(int presetIndex)
{
    loadPresetToSelectedTrack();
}

void InstrumentBrowserPanel::loadPresetToSelectedTrack()
{
    // Get selected preset
    int selectedRow = presetList.getSelectedRow();
    if (selectedRow < 0 || selectedRow >= visiblePresetIndices_.size())
    {
        showStatus("Please select a preset", true);
        return;
    }

    int actualIndex = visiblePresetIndices_[selectedRow].getIntValue();
    if (actualIndex < 0 || actualIndex >= presetItems_.size())
        return;

    const auto& presetItem = presetItems_[actualIndex];

    // Get selected track
    auto* track = getSelectedTrack();
    if (track == nullptr)
    {
        showStatus("Please select an instrument track", true);
        return;
    }

    // Check if track is an instrument track or has an instrument
    if (track->getType() != Track::Type::Instrument && !track->hasInstrument())
    {
        showStatus("Selected track is not an instrument track", true);
        return;
    }

    // Get or create instrument
    Instrument* instrument = track->getInstrument();
    if (instrument == nullptr)
    {
        // Create new instrument instance
        auto newInstrument = instrumentRegistry_.createInstrument(selectedInstrumentId_);
        if (newInstrument == nullptr)
        {
            showStatus("Failed to create instrument: " + selectedInstrumentId_, true);
            return;
        }

        instrument = newInstrument.get();
        track->setInstrument(std::move(newInstrument));
    }

    // Apply preset to instrument
    bool success = instrument->applyPreset(presetItem.preset);
    if (success)
    {
        showStatus("Loaded preset: " + juce::String(presetItem.preset.name));
    }
    else
    {
        showStatus("Failed to load preset", true);
    }
}

void InstrumentBrowserPanel::updateSearchFilter()
{
    applyFilters();
}

void InstrumentBrowserPanel::updateTagFilter(const juce::String& tag)
{
    if (tag == "All")
    {
        activeTag = "";  // Show all
    }
    else
    {
        activeTag = tag;
    }

    applyFilters();
}

void InstrumentBrowserPanel::applyFilters()
{
    juce::String searchTerm = searchBox.getText().toLowerCase().trim();

    // Update filter flags
    for (auto& item : presetItems_)
    {
        // Search filter: match name or tags
        bool matchesSearch = searchTerm.isEmpty();
        if (!matchesSearch)
        {
            juce::String presetName = juce::String(item.preset.name).toLowerCase();
            matchesSearch = presetName.contains(searchTerm);

            // Also check tags
            if (!matchesSearch)
            {
                for (const auto& tag : item.preset.tags)
                {
                    if (juce::String(tag).toLowerCase().contains(searchTerm))
                    {
                        matchesSearch = true;
                        break;
                    }
                }
            }

            // Also check category
            if (!matchesSearch)
            {
                juce::String category = juce::String(item.preset.category).toLowerCase();
                matchesSearch = category.contains(searchTerm);
            }
        }

        item.matchesSearch = matchesSearch;

        // Tag filter: match tag or category
        bool matchesTag = activeTag.isEmpty();
        if (!matchesTag)
        {
            juce::String tagLower = activeTag.toLowerCase();

            // Check tags
            for (const auto& tag : item.preset.tags)
            {
                if (juce::String(tag).toLowerCase() == tagLower)
                {
                    matchesTag = true;
                    break;
                }
            }

            // Also check category
            if (!matchesTag)
            {
                juce::String category = juce::String(item.preset.category).toLowerCase();
                matchesTag = (category == tagLower);
            }
        }

        item.matchesTag = matchesTag;
    }

    // Build visible preset indices
    visiblePresetIndices_.clear();
    for (int i = 0; i < presetItems_.size(); ++i)
    {
        if (presetItems_[i].isVisible())
        {
            visiblePresetIndices_.add(juce::String(i));
        }
    }

    // Update preset list
    presetList.updateContent();
    presetList.repaint();
}

void InstrumentBrowserPanel::showStatus(const juce::String& message, bool isError)
{
    statusLabel.setText(message, juce::dontSendNotification);
    statusLabel.setColour(juce::Label::backgroundColourId,
                          isError ? juce::Colour(0xff8B0000) : juce::Colour(0xff2a2a2a));
    statusLabel.setColour(juce::Label::outlineColourId,
                          isError ? juce::Colours::red : juce::Colour(0xff4a9eff));
    statusLabel.setVisible(true);
    statusLabelAlpha = 255;

    // Start fade-out timer (using inherited Timer functionality)
    startTimer(50);  // 50ms updates for smooth fade
}

void InstrumentBrowserPanel::timerCallback()
{
    // Fade out status label
    if (statusLabelAlpha > 0)
    {
        // Hold at full opacity for 2 seconds (2000ms / 50ms = 40 ticks)
        static int holdTicks = 0;
        if (holdTicks < 40)
        {
            holdTicks++;
            return;
        }

        // Fade out over 1 second (1000ms / 50ms = 20 ticks)
        statusLabelAlpha -= 12;  // 255 / 20 ≈ 12
        if (statusLabelAlpha <= 0)
        {
            statusLabelAlpha = 0;
            statusLabel.setVisible(false);
            stopTimer();  // Stop inherited Timer
            holdTicks = 0;
        }

        statusLabel.setAlpha(statusLabelAlpha / 255.0f);
        statusLabel.repaint();
    }
}

zenith::Track* InstrumentBrowserPanel::getSelectedTrack() const
{
    // Get first instrument track from engine
    // TODO: Implement proper track selection (selected track in arranger)
    // For now, we'll use the first instrument track found in the engine
    
    int numTracks = engine_.getNumTracks();
    const auto& tracks = engine_.tracks();
    
    for (const auto& track : tracks)
    {
        if (track->getType() == Track::Type::Instrument)
        {
            return track.get();
        }
    }

    return nullptr;
}

//==============================================================================
// InstrumentListBoxModel Implementation
//==============================================================================

InstrumentBrowserPanel::InstrumentListBoxModel::InstrumentListBoxModel(InstrumentBrowserPanel& owner)
    : owner_(owner)
{
}

int InstrumentBrowserPanel::InstrumentListBoxModel::getNumRows()
{
    return owner_.instrumentIds_.size();
}

void InstrumentBrowserPanel::InstrumentListBoxModel::paintListBoxItem(
    int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= owner_.instrumentIds_.size())
        return;

    // Background
    if (rowIsSelected)
        g.fillAll(juce::Colour(0xff4a9eff).withAlpha(0.3f));
    else if (rowNumber % 2 == 0)
        g.fillAll(juce::Colour(0xff242424));
    else
        g.fillAll(juce::Colour(0xff1a1a1a));

    // Get instrument metadata
    auto instrumentId = owner_.instrumentIds_[rowNumber];
    InstrumentMetadata metadata;
    bool hasMetadata = owner_.instrumentRegistry_.getMetadata(instrumentId, metadata);

    if (hasMetadata)
    {
        // Draw instrument name
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(14.0f));
        g.drawText(metadata.name, 10, 0, width - 20, height,
                   juce::Justification::centredLeft, true);

        // Draw category (smaller, grey)
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(11.0f));
        g.drawText(metadata.category, 10, 0, width - 20, height,
                   juce::Justification::centredRight, true);
    }
    else
    {
        // Fallback: just draw ID
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(14.0f));
        g.drawText(instrumentId, 10, 0, width - 20, height,
                   juce::Justification::centredLeft, true);
    }
}

void InstrumentBrowserPanel::InstrumentListBoxModel::listBoxItemClicked(
    int row, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    owner_.onInstrumentSelected(row);
}

//==============================================================================
// PresetListBoxModel Implementation
//==============================================================================

InstrumentBrowserPanel::PresetListBoxModel::PresetListBoxModel(InstrumentBrowserPanel& owner)
    : owner_(owner)
{
}

int InstrumentBrowserPanel::PresetListBoxModel::getNumRows()
{
    return owner_.visiblePresetIndices_.size();
}

void InstrumentBrowserPanel::PresetListBoxModel::paintListBoxItem(
    int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= owner_.visiblePresetIndices_.size())
        return;

    int actualIndex = owner_.visiblePresetIndices_[rowNumber].getIntValue();
    if (actualIndex < 0 || actualIndex >= owner_.presetItems_.size())
        return;

    const auto& preset = owner_.presetItems_[actualIndex].preset;

    // Background
    if (rowIsSelected)
        g.fillAll(juce::Colour(0xff4a9eff).withAlpha(0.3f));
    else if (rowNumber % 2 == 0)
        g.fillAll(juce::Colour(0xff242424));
    else
        g.fillAll(juce::Colour(0xff1a1a1a));

    // Draw preset name
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(13.0f));
    g.drawText(preset.name, 10, 0, width - 80, height,
               juce::Justification::centredLeft, true);

    // Draw category badge
    if (!preset.category.empty())
    {
        g.setColour(juce::Colour(0xff4a9eff).withAlpha(0.5f));
        juce::Rectangle<int> badge(width - 75, 4, 65, height - 8);
        g.fillRoundedRectangle(badge.toFloat(), 3.0f);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(10.0f));
        g.drawText(preset.category, badge, juce::Justification::centred, true);
    }
}

void InstrumentBrowserPanel::PresetListBoxModel::listBoxItemDoubleClicked(
    int row, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    owner_.onPresetDoubleClicked(row);
}

} // namespace zenith
