/**
 * @file InstrumentBrowserPanel.cpp
 * @brief Instrument & Preset Browser UI implementation
 * 
 * DESIGN SYSTEM: Updated to use ZenithLookAndFeel design tokens
 */

#include "InstrumentBrowserPanel.h"
#include "../../include/Engine.h"
#include "ZenithLookAndFeel.h"  // DESIGN SYSTEM: Include for design tokens
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
#ifdef ZENITH_USE_SKIA
    // ===== SKIA VERSION =====

    // Title header
    titleLabel.setText("Instruments", juce::dontSendNotification);
    titleLabel.setTextStyle(TextStyle::Heading);
    addAndMakeVisible(titleLabel);

    // Search section
    searchLabel.setText("Search:", juce::dontSendNotification);
    addAndMakeVisible(searchLabel);

    searchBox.setPlaceholder("Search presets...");
    searchBox.onTextChange = [this](const juce::String&) { updateSearchFilter(); };
    addAndMakeVisible(searchBox);

    // Tag chips
    tagsLabel.setText("Tags:", juce::dontSendNotification);
    addAndMakeVisible(tagsLabel);

    addAndMakeVisible(tagChipsContainer);
    initializeTagChips();

    // Instrument list
    instrumentsLabel.setText("Instruments:", juce::dontSendNotification);
    instrumentsLabel.setTextStyle(TextStyle::Bold);
    addAndMakeVisible(instrumentsLabel);

    instrumentList.setRowHeight(30.0f);
    instrumentList.setItemPaintCallback([this](SkCanvas* canvas, SkRect bounds, int row, bool isSelected, bool isHovered) {
        if (row < 0 || row >= instrumentIds_.size())
            return;

        const auto& theme = SkiaTheme::getInstance();
        const auto& colors = theme.getColors();

        // Get instrument info
        juce::String instId = instrumentIds_[row];
        InstrumentMetadata metadata;
        bool hasMetadata = instrumentRegistry_.getMetadata(instId, metadata);
        juce::String displayName = hasMetadata ? metadata.name : instId;

        // Render text
        SkiaTextRenderer textRenderer;
        TextRenderOptions opts;
        opts.color = isSelected ? colors.textOnAccent : colors.textPrimary;
        opts.antiAlias = true;
        opts.effects = TextEffect::None;

        float textX = bounds.x() + 8.0f;
        float textY = bounds.centerY() + 4.0f;

        textRenderer.drawText(canvas, displayName, textX, textY, TextStyle::Regular, opts);
    });

    instrumentList.onRowSelected = [this](int row) { onInstrumentSelected(row); };
    addAndMakeVisible(instrumentList);

    // Preset list
    presetsLabel.setText("Presets:", juce::dontSendNotification);
    presetsLabel.setTextStyle(TextStyle::Bold);
    addAndMakeVisible(presetsLabel);

    presetList.setRowHeight(28.0f);
    presetList.setItemPaintCallback([this](SkCanvas* canvas, SkRect bounds, int row, bool isSelected, bool isHovered) {
        if (row < 0 || row >= static_cast<int>(presetItems_.size()))
            return;

        const auto& theme = SkiaTheme::getInstance();
        const auto& colors = theme.getColors();

        // Get visible preset
        int visibleCount = 0;
        for (size_t i = 0; i < presetItems_.size(); ++i)
        {
            if (presetItems_[i].isVisible())
            {
                if (visibleCount == row)
                {
                    // Render this preset
                    SkiaTextRenderer textRenderer;
                    TextRenderOptions opts;
                    opts.color = isSelected ? colors.textOnAccent : colors.textPrimary;
                    opts.antiAlias = true;
                    opts.effects = TextEffect::None;

                    float textX = bounds.x() + 8.0f;
                    float textY = bounds.centerY() + 4.0f;

                    textRenderer.drawText(canvas, presetItems_[i].preset.name, textX, textY, TextStyle::Regular, opts);
                    break;
                }
                visibleCount++;
            }
        }
    });

    presetList.onRowDoubleClicked = [this](int row) { onPresetDoubleClicked(row); };
    addAndMakeVisible(presetList);

    // Load preset button
    loadPresetButton.setButtonText("Load to Track");
    loadPresetButton.onClick = [this]() { loadPresetToSelectedTrack(); };
    addAndMakeVisible(loadPresetButton);

    // Status label (toast)
    statusLabel.setVisible(false);
    addAndMakeVisible(statusLabel);

#else
    // ===== JUCE FALLBACK VERSION - DESIGN SYSTEM: Use tokens =====

    // DESIGN SYSTEM: Title using textPrimary and Typography
    titleLabel.setText("Instruments", juce::dontSendNotification);
    titleLabel.setFont(ZenithLookAndFeel::Typography::getH3());
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    addAndMakeVisible(titleLabel);

    // DESIGN SYSTEM: Search label using textSecondary
    searchLabel.setText("Search:", juce::dontSendNotification);
    searchLabel.setFont(ZenithLookAndFeel::Typography::getBody());
    searchLabel.setColour(juce::Label::textColourId, juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
    addAndMakeVisible(searchLabel);

    // DESIGN SYSTEM: Search box using elevation and border tokens
    searchBox.setMultiLine(false);
    searchBox.setReturnKeyStartsNewLine(false);
    searchBox.setTextToShowWhenEmpty("Search presets...", juce::Colour(ZenithLookAndFeel::Colors::textDisabled));
    searchBox.setFont(ZenithLookAndFeel::Typography::getBody());
    searchBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(ZenithLookAndFeel::Elevation::dp4));
    searchBox.setColour(juce::TextEditor::textColourId, juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    searchBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(ZenithLookAndFeel::Colors::borderSubtle));
    searchBox.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(ZenithLookAndFeel::Colors::accentPrimary));
    searchBox.addListener(this);
    addAndMakeVisible(searchBox);

    // DESIGN SYSTEM: Tags label
    tagsLabel.setText("Tags:", juce::dontSendNotification);
    tagsLabel.setFont(ZenithLookAndFeel::Typography::getBody());
    tagsLabel.setColour(juce::Label::textColourId, juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
    addAndMakeVisible(tagsLabel);

    addAndMakeVisible(tagChipsContainer);
    initializeTagChips();

    // DESIGN SYSTEM: Instruments label
    instrumentsLabel.setText("Instruments:", juce::dontSendNotification);
    instrumentsLabel.setFont(ZenithLookAndFeel::Typography::getBodyBold());
    instrumentsLabel.setColour(juce::Label::textColourId, juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    addAndMakeVisible(instrumentsLabel);

    // DESIGN SYSTEM: Instrument list using elevation tokens
    instrumentListModel_ = std::make_unique<InstrumentListBoxModel>(*this);
    instrumentList.setModel(instrumentListModel_.get());
    instrumentList.setColour(juce::ListBox::backgroundColourId, juce::Colour(ZenithLookAndFeel::Elevation::dp1));
    instrumentList.setColour(juce::ListBox::outlineColourId, juce::Colour(ZenithLookAndFeel::Colors::borderSubtle));
    instrumentList.setRowHeight(30);
    addAndMakeVisible(instrumentList);

    // DESIGN SYSTEM: Presets label
    presetsLabel.setText("Presets:", juce::dontSendNotification);
    presetsLabel.setFont(ZenithLookAndFeel::Typography::getBodyBold());
    presetsLabel.setColour(juce::Label::textColourId, juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    addAndMakeVisible(presetsLabel);

    // DESIGN SYSTEM: Preset list using elevation tokens
    presetListModel_ = std::make_unique<PresetListBoxModel>(*this);
    presetList.setModel(presetListModel_.get());
    presetList.setColour(juce::ListBox::backgroundColourId, juce::Colour(ZenithLookAndFeel::Elevation::dp1));
    presetList.setColour(juce::ListBox::outlineColourId, juce::Colour(ZenithLookAndFeel::Colors::borderSubtle));
    presetList.setRowHeight(28);
    addAndMakeVisible(presetList);

    loadPresetButton.setButtonText("Load to Track");
    loadPresetButton.setEnabled(false);
    loadPresetButton.onClick = [this]() { loadPresetToSelectedTrack(); };
    addAndMakeVisible(loadPresetButton);

    // DESIGN SYSTEM: Status label using elevation and accent tokens
    statusLabel.setFont(ZenithLookAndFeel::Typography::getSmall());
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::backgroundColourId, juce::Colour(ZenithLookAndFeel::Elevation::dp4));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    statusLabel.setColour(juce::Label::outlineColourId, juce::Colour(ZenithLookAndFeel::Colors::accentPrimary));
    statusLabel.setVisible(false);
    addAndMakeVisible(statusLabel);
#endif

    // Load instruments (common to both versions)
    instrumentIds_ = instrumentRegistry_.getInstrumentIds();

#ifdef ZENITH_USE_SKIA
    instrumentList.setNumRows(instrumentIds_.size());
#else
    instrumentList.updateContent();
#endif

    setSize(300, 600);
}

InstrumentBrowserPanel::~InstrumentBrowserPanel()
{
#ifndef ZENITH_USE_SKIA
    searchBox.removeListener(this);
#endif
}

void InstrumentBrowserPanel::paint(juce::Graphics& g)
{
    // DESIGN SYSTEM: Background using elevation token
    g.fillAll(juce::Colour(ZenithLookAndFeel::Elevation::dp2));

    // DESIGN SYSTEM: Border using borderSubtle token
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::borderSubtle));
    g.drawRect(getLocalBounds(), 1);
}

void InstrumentBrowserPanel::resized()
{
    auto bounds = getLocalBounds().reduced(ZenithLookAndFeel::Metrics::s);

    // Title
    titleLabel.setBounds(bounds.removeFromTop(30));

    bounds.removeFromTop(ZenithLookAndFeel::Metrics::s);  // Spacing

    // Search section
    searchLabel.setBounds(bounds.removeFromTop(20));
    searchBox.setBounds(bounds.removeFromTop(30));

    bounds.removeFromTop(ZenithLookAndFeel::Metrics::s);  // Spacing

    // Tag chips section
    tagsLabel.setBounds(bounds.removeFromTop(20));
    auto tagChipsArea = bounds.removeFromTop(80);  // 2 rows of chips
    tagChipsContainer.setBounds(tagChipsArea);

    // Layout tag chips in a grid (5 per row)
    int chipWidth = 55;
    int chipHeight = 28;
    int chipSpacing = ZenithLookAndFeel::Metrics::xs;
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

    bounds.removeFromTop(ZenithLookAndFeel::Metrics::s);  // Spacing

    // Instrument list section
    instrumentsLabel.setBounds(bounds.removeFromTop(20));
    auto instrumentListArea = bounds.removeFromTop(150);
    instrumentList.setBounds(instrumentListArea);

    bounds.removeFromTop(ZenithLookAndFeel::Metrics::s);  // Spacing

    // Preset list section
    presetsLabel.setBounds(bounds.removeFromTop(20));

    // Load button at bottom
    auto loadButtonArea = bounds.removeFromBottom(35);
    loadPresetButton.setBounds(loadButtonArea);

    bounds.removeFromBottom(ZenithLookAndFeel::Metrics::xs);  // Spacing

    // Preset list takes remaining space
    presetList.setBounds(bounds);

    // Status label (overlay at bottom)
    statusLabel.setBounds(getLocalBounds().removeFromBottom(40).reduced(ZenithLookAndFeel::Metrics::m, ZenithLookAndFeel::Metrics::s));
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
// TextEditor::Listener (JUCE only)
//==============================================================================

#ifndef ZENITH_USE_SKIA
void InstrumentBrowserPanel::textEditorTextChanged(juce::TextEditor& editor)
{
    if (&editor == &searchBox)
    {
        updateSearchFilter();
    }
}
#endif

//==============================================================================
// Internal methods
//==============================================================================

void InstrumentBrowserPanel::initializeTagChips()
{
#ifdef ZENITH_USE_SKIA
    for (const auto& tagName : TAG_CHIPS)
    {
        auto chip = std::make_unique<SkiaButtonNative>(tagName, SkiaButtonNative::Style::Secondary);
        auto* chipPtr = chip.get();  // Capture raw pointer
        chip->onClick = [this, tagName, chipPtr]() {
            // Deselect all other chips
            for (auto& otherChip : tagChips)
            {
                if (otherChip.get() != chipPtr)
                {
                    otherChip->setStyle(SkiaButtonNative::Style::Secondary);
                }
            }
            // Highlight this chip
            chipPtr->setStyle(SkiaButtonNative::Style::Primary);
            updateTagFilter(tagName);
        };

        tagChipsContainer.addAndMakeVisible(chip.get());
        tagChips.push_back(std::move(chip));
    }

    // Select "All" by default
    if (!tagChips.empty())
    {
        tagChips[0]->setStyle(SkiaButtonNative::Style::Primary);
        activeTag = "";  // Empty = show all
    }
#else
    // DESIGN SYSTEM: Tag chips using elevation and accent tokens
    for (const auto& tagName : TAG_CHIPS)
    {
        auto chip = std::make_unique<juce::TextButton>(tagName);
        chip->setColour(juce::TextButton::buttonColourId, juce::Colour(ZenithLookAndFeel::Elevation::dp4));
        chip->setColour(juce::TextButton::buttonOnColourId, juce::Colour(ZenithLookAndFeel::Colors::accentPrimary));
        chip->setColour(juce::TextButton::textColourOffId, juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
        chip->setColour(juce::TextButton::textColourOnId, juce::Colour(ZenithLookAndFeel::Colors::textOnAccent));

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
#endif
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
#ifndef ZENITH_USE_SKIA
        presetList.updateContent();
#endif
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
#ifdef ZENITH_USE_SKIA
    // Count visible presets for Skia list
    int visibleCount = 0;
    for (const auto& item : presetItems_)
    {
        if (item.isVisible())
            visibleCount++;
    }
    presetList.setNumRows(visibleCount);
#else
    presetList.updateContent();
#endif
    presetList.repaint();
}

void InstrumentBrowserPanel::showStatus(const juce::String& message, bool isError)
{
    statusLabel.setText(message, juce::dontSendNotification);
    // DESIGN SYSTEM: Use danger color for errors, elevation for normal
    statusLabel.setColour(juce::Label::backgroundColourId,
                          isError ? juce::Colour(ZenithLookAndFeel::Colors::danger).darker(0.5f) 
                                  : juce::Colour(ZenithLookAndFeel::Elevation::dp4));
    statusLabel.setColour(juce::Label::outlineColourId,
                          isError ? juce::Colour(ZenithLookAndFeel::Colors::danger) 
                                  : juce::Colour(ZenithLookAndFeel::Colors::accentPrimary));
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
// ListBoxModel Implementations (JUCE only)
//==============================================================================

#ifndef ZENITH_USE_SKIA

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

    // DESIGN SYSTEM: Background using elevation tokens
    if (rowIsSelected)
        g.fillAll(juce::Colour(ZenithLookAndFeel::Colors::accentPrimary).withAlpha(0.3f));
    else if (rowNumber % 2 == 0)
        g.fillAll(juce::Colour(ZenithLookAndFeel::Elevation::dp2));
    else
        g.fillAll(juce::Colour(ZenithLookAndFeel::Elevation::dp1));

    // Get instrument metadata
    auto instrumentId = owner_.instrumentIds_[rowNumber];
    InstrumentMetadata metadata;
    bool hasMetadata = owner_.instrumentRegistry_.getMetadata(instrumentId, metadata);

    if (hasMetadata)
    {
        // DESIGN SYSTEM: Draw instrument name using textPrimary
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
        g.setFont(ZenithLookAndFeel::Typography::getBody());
        g.drawText(metadata.name, ZenithLookAndFeel::Spacing::s, 0, width - 20, height,
                   juce::Justification::centredLeft, true);

        // DESIGN SYSTEM: Draw category using textSecondary
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
        g.setFont(ZenithLookAndFeel::Typography::getSmall());
        g.drawText(metadata.category, ZenithLookAndFeel::Spacing::s, 0, width - 20, height,
                   juce::Justification::centredRight, true);
    }
    else
    {
        // Fallback: just draw ID
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
        g.setFont(ZenithLookAndFeel::Typography::getBody());
        g.drawText(instrumentId, ZenithLookAndFeel::Spacing::s, 0, width - 20, height,
                   juce::Justification::centredLeft, true);
    }
}

void InstrumentBrowserPanel::InstrumentListBoxModel::listBoxItemClicked(
    int row, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    owner_.onInstrumentSelected(row);
}

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

    // DESIGN SYSTEM: Background using elevation tokens
    if (rowIsSelected)
        g.fillAll(juce::Colour(ZenithLookAndFeel::Colors::accentPrimary).withAlpha(0.3f));
    else if (rowNumber % 2 == 0)
        g.fillAll(juce::Colour(ZenithLookAndFeel::Elevation::dp2));
    else
        g.fillAll(juce::Colour(ZenithLookAndFeel::Elevation::dp1));

    // DESIGN SYSTEM: Draw preset name using textPrimary
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    g.setFont(ZenithLookAndFeel::Typography::getSmall());
    g.drawText(preset.name, ZenithLookAndFeel::Spacing::s, 0, width - 80, height,
               juce::Justification::centredLeft, true);

    // DESIGN SYSTEM: Draw category badge using accentPrimary
    if (!preset.category.empty())
    {
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::accentPrimary).withAlpha(0.5f));
        juce::Rectangle<int> badge(width - 75, 4, 65, height - 8);
        g.fillRoundedRectangle(badge.toFloat(), ZenithLookAndFeel::Radius::xs);

        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
        g.setFont(ZenithLookAndFeel::Typography::getTiny());
        g.drawText(preset.category, badge, juce::Justification::centred, true);
    }
}

void InstrumentBrowserPanel::PresetListBoxModel::listBoxItemDoubleClicked(
    int row, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    owner_.onPresetDoubleClicked(row);
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
