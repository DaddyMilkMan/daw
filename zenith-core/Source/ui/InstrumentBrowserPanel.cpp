/**
 * @file InstrumentBrowserPanel.cpp
 * @brief Instrument & Preset Browser UI implementation
 */

#include "InstrumentBrowserPanel.h"
#include "ZenithLookAndFeel.h"
#include "AudioFeedback.h"
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
    // Start 60Hz animation timer
    startTimerHz(60);

    // Title header
    titleLabel.setText("Instruments", juce::dontSendNotification);
    titleLabel.setFont(ZenithLookAndFeel::getFontTitle());
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    addAndMakeVisible(titleLabel);

    // Search box with enhanced styling
    searchLabel.setText("Search:", juce::dontSendNotification);
    searchLabel.setFont(ZenithLookAndFeel::getFontBody());
    searchLabel.setColour(juce::Label::textColourId, juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
    addAndMakeVisible(searchLabel);

    searchBox.setMultiLine(false);
    searchBox.setReturnKeyStartsNewLine(false);
    searchBox.setTextToShowWhenEmpty("Search...", juce::Colour(ZenithLookAndFeel::Colors::textDisabled));
    searchBox.setFont(ZenithLookAndFeel::getFontBody());
    searchBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(ZenithLookAndFeel::Colors::backgroundDark));
    searchBox.setColour(juce::TextEditor::textColourId, juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    searchBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(ZenithLookAndFeel::Colors::border));
    searchBox.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(ZenithLookAndFeel::Colors::accentPrimary));
    searchBox.addListener(this);
    addAndMakeVisible(searchBox);

#ifdef ZENITH_USE_SKIA
    // GPU-accelerated clear button with spring physics
    searchClearButton = std::make_unique<SkiaButtonComponent>("×", SkiaButtonComponent::Style::Secondary);
    searchClearButton->onClick = [this]() { clearSearch(); };
    searchClearButton->setVisible(false);
    addAndMakeVisible(*searchClearButton);
#else
    // Fallback: JUCE clear button
    searchClearButton.setButtonText("x"); // Simple x, will be styled by LookAndFeel
    searchClearButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    searchClearButton.setColour(juce::TextButton::textColourOffId, juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
    searchClearButton.onClick = [this]() { clearSearch(); };
    searchClearButton.setVisible(false);
    addAndMakeVisible(searchClearButton);
#endif

    // Tag chips with enhanced styling
    tagsLabel.setText("Categories:", juce::dontSendNotification);
    tagsLabel.setFont(ZenithLookAndFeel::getFontBody());
    tagsLabel.setColour(juce::Label::textColourId, juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
    addAndMakeVisible(tagsLabel);

    addAndMakeVisible(tagChipsContainer);
    initializeTagChips();

    // Instrument list
    instrumentsLabel.setText("Instruments:", juce::dontSendNotification);
    instrumentsLabel.setFont(ZenithLookAndFeel::getFontHeading());
    instrumentsLabel.setColour(juce::Label::textColourId, juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    addAndMakeVisible(instrumentsLabel);

    instrumentListModel_ = std::make_unique<InstrumentListBoxModel>(*this);
    instrumentList.setModel(instrumentListModel_.get());
    instrumentList.setColour(juce::ListBox::backgroundColourId, juce::Colour(ZenithLookAndFeel::Colors::backgroundDark));
    instrumentList.setColour(juce::ListBox::outlineColourId, juce::Colour(ZenithLookAndFeel::Colors::border));
    instrumentList.setRowHeight(32); // Match buttonHeightM
    addAndMakeVisible(instrumentList);

    // Preset list
    presetsLabel.setText("Presets:", juce::dontSendNotification);
    presetsLabel.setFont(ZenithLookAndFeel::getFontHeading());
    presetsLabel.setColour(juce::Label::textColourId, juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    addAndMakeVisible(presetsLabel);

    presetListModel_ = std::make_unique<PresetListBoxModel>(*this);
    presetList.setModel(presetListModel_.get());
    presetList.setColour(juce::ListBox::backgroundColourId, juce::Colour(ZenithLookAndFeel::Colors::backgroundDark));
    presetList.setColour(juce::ListBox::outlineColourId, juce::Colour(ZenithLookAndFeel::Colors::border));
    presetList.setRowHeight(28);
    addAndMakeVisible(presetList);

#ifdef ZENITH_USE_SKIA
    // GPU-accelerated load button with spring physics
    loadPresetButton = std::make_unique<SkiaButtonComponent>("Load to Track", SkiaButtonComponent::Style::Primary);
    loadPresetButton->onClick = [this]() { loadPresetToSelectedTrack(); };
    addAndMakeVisible(*loadPresetButton);
#else
    // Fallback: JUCE load button
    loadPresetButton.setButtonText("Load to Track");
    loadPresetButton.setEnabled(false);
    loadPresetButton.onClick = [this]() { loadPresetToSelectedTrack(); };
    addAndMakeVisible(loadPresetButton);
#endif

    // Status label (toast)
    statusLabel.setFont(ZenithLookAndFeel::getFontSmall());
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::backgroundColourId, juce::Colour(ZenithLookAndFeel::Colors::backgroundLight));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    statusLabel.setColour(juce::Label::outlineColourId, juce::Colour(ZenithLookAndFeel::Colors::border));
    statusLabel.setVisible(false);
    addAndMakeVisible(statusLabel);

    // Load instruments
    instrumentIds_ = instrumentRegistry_.getInstrumentIds();
    instrumentList.updateContent();

    setSize(ZenithLookAndFeel::Metrics::browserPanelWidth, 600);
}

InstrumentBrowserPanel::~InstrumentBrowserPanel()
{
    searchBox.removeListener(this);
    stopTimer();
}

void InstrumentBrowserPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(ZenithLookAndFeel::Colors::backgroundMid));

    // Right border
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::border));
    g.drawRect(bounds.removeFromRight(1), 1);

    // Draw focus glow around search box if focused
    if (searchFocusAnimation > 0.01f)
    {
        auto searchBounds = searchBox.getBounds().toFloat().expanded(2.0f);
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::accentPrimary).withAlpha(searchFocusAnimation * 0.3f));
        g.drawRoundedRectangle(searchBounds, ZenithLookAndFeel::Metrics::radiusM, 2.0f);
    }
}

void InstrumentBrowserPanel::resized()
{
    using namespace ZenithLookAndFeel::Metrics;
    auto bounds = getLocalBounds().reduced(spacingS);

    // Title
    titleLabel.setBounds(bounds.removeFromTop(buttonHeightM));

    bounds.removeFromTop(spacingS);

    // Search section
    searchLabel.setBounds(bounds.removeFromTop(20));
    auto searchArea = bounds.removeFromTop(buttonHeightM);
    searchBox.setBounds(searchArea);

    // Clear button overlays search box on the right
    if (!searchBox.isEmpty())
    {
#ifdef ZENITH_USE_SKIA
        searchClearButton->setBounds(searchArea.getRight() - 24, searchArea.getY() + 4, 20, 20);
#else
        searchClearButton.setBounds(searchArea.getRight() - 24, searchArea.getY() + 4, 20, 20);
#endif
    }

    bounds.removeFromTop(spacingS);

    // Tag chips section
    tagsLabel.setBounds(bounds.removeFromTop(20));
    auto tagChipsArea = bounds.removeFromTop(80);  // 2 rows of chips
    tagChipsContainer.setBounds(tagChipsArea);

    // Layout tag chips in a grid (4 per row to fit width)
    int chipWidth = 60;
    int chipHeight = 24;
    int chipSpacing = spacingXS;
    int x = 0;
    int y = 0;

    for (int i = 0; i < tagChips.size(); ++i)
    {
        tagChips[i]->setBounds(x, y, chipWidth, chipHeight);

        x += chipWidth + chipSpacing;
        if ((i + 1) % 4 == 0)  // New row every 4 chips
        {
            x = 0;
            y += chipHeight + chipSpacing;
        }
    }

    bounds.removeFromTop(spacingS);

    // Instrument list section
    instrumentsLabel.setBounds(bounds.removeFromTop(20));
    auto instrumentListArea = bounds.removeFromTop(150);
    instrumentList.setBounds(instrumentListArea);

    bounds.removeFromTop(spacingS);

    // Preset list section
    presetsLabel.setBounds(bounds.removeFromTop(20));

    // Load button at bottom
    auto loadButtonArea = bounds.removeFromBottom(buttonHeightL);
#ifdef ZENITH_USE_SKIA
    loadPresetButton->setBounds(loadButtonArea);
#else
    loadPresetButton.setBounds(loadButtonArea);
#endif

    bounds.removeFromBottom(spacingS);

    // Preset list takes remaining space
    presetList.setBounds(bounds);

    // Status label (overlay at bottom)
    statusLabel.setBounds(getLocalBounds().removeFromBottom(40).reduced(spacingM, spacingS));
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

        // Show/hide clear button based on text content
        bool hasText = !searchBox.isEmpty();
#ifdef ZENITH_USE_SKIA
        searchClearButton->setVisible(hasText);
        if (hasText)
        {
            auto searchArea = searchBox.getBounds();
            searchClearButton->setBounds(searchArea.getRight() - 24, searchArea.getY() + 4, 20, 20);
        }
#else
        searchClearButton.setVisible(hasText);
        if (hasText)
        {
            auto searchArea = searchBox.getBounds();
            searchClearButton.setBounds(searchArea.getRight() - 24, searchArea.getY() + 4, 20, 20);
        }
#endif
    }
}

//==============================================================================
// Internal methods
//==============================================================================

void InstrumentBrowserPanel::initializeTagChips()
{
#ifdef ZENITH_USE_SKIA
    // GPU-accelerated tag chips with spring physics
    for (const auto& tagName : TAG_CHIPS)
    {
        auto chip = std::make_unique<SkiaButtonComponent>(tagName, SkiaButtonComponent::Style::Secondary);
        chip->setToggleable(true);

        chip->onClick = [this, tagName, chipPtr = chip.get()]() {
            // Deselect all other chips (radio behavior)
            for (auto& otherChip : tagChips)
            {
                if (otherChip.get() != chipPtr)
                    otherChip->setToggleState(false);
            }
            chipPtr->setToggleState(true);
            updateTagFilter(tagName);
        };

        tagChipsContainer.addAndMakeVisible(chip.get());
        tagChips.push_back(std::move(chip));
    }

    // Select "All" by default
    if (!tagChips.empty())
    {
        tagChips[0]->setToggleState(true);
        activeTag = "";  // Empty = show all
    }
#else
    // Fallback: JUCE tag chips
    for (const auto& tagName : TAG_CHIPS)
    {
        auto chip = std::make_unique<juce::TextButton>(tagName);

        // Zenith-style chip styling
        chip->setColour(juce::TextButton::buttonColourId, juce::Colour(ZenithLookAndFeel::Colors::backgroundPanel));
        chip->setColour(juce::TextButton::buttonOnColourId, juce::Colour(ZenithLookAndFeel::Colors::accentPrimary));
        chip->setColour(juce::TextButton::textColourOffId, juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
        chip->setColour(juce::TextButton::textColourOnId, juce::Colour(ZenithLookAndFeel::Colors::backgroundDark)); // Dark text on bright accent

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

        // Play success chime
        AudioFeedback::getInstance().playSound(AudioFeedback::Success, 0.3f);
    }
    else
    {
        showStatus("Failed to load preset", true);

        // Play error beep
        AudioFeedback::getInstance().playSound(AudioFeedback::Error, 0.3f);
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
                          isError ? juce::Colour(ZenithLookAndFeel::Colors::accentDanger).withAlpha(0.2f) 
                                  : juce::Colour(ZenithLookAndFeel::Colors::backgroundLight));
    statusLabel.setColour(juce::Label::outlineColourId,
                          isError ? juce::Colour(ZenithLookAndFeel::Colors::accentDanger) 
                                  : juce::Colour(ZenithLookAndFeel::Colors::accentPrimary));
    statusLabel.setVisible(true);
    statusLabelAlpha = 255;
    statusHoldTicks = 0;
}

void InstrumentBrowserPanel::timerCallback()
{
    // Update search focus animation (60Hz)
    const float animationSpeed = 0.15f;
    float targetFocus = searchHasFocus ? 1.0f : 0.0f;
    searchFocusAnimation += (targetFocus - searchFocusAnimation) * animationSpeed;

    if (std::abs(searchFocusAnimation - targetFocus) > 0.01f)
        repaint();

    // Update status animation
    updateStatusAnimation();
}

void InstrumentBrowserPanel::updateStatusAnimation()
{
    // Fade out status label
    if (statusLabelAlpha > 0)
    {
        // Hold at full opacity for 2 seconds (2000ms / ~16.67ms = 120 ticks at 60Hz)
        if (statusHoldTicks < 120)
        {
            statusHoldTicks++;
            return;
        }

        // Fade out over 0.5 seconds (30 ticks at 60Hz)
        statusLabelAlpha -= 8;  // 255 / 30 ≈ 8
        if (statusLabelAlpha <= 0)
        {
            statusLabelAlpha = 0;
            statusLabel.setVisible(false);
            statusHoldTicks = 0;
        }

        statusLabel.setAlpha(statusLabelAlpha / 255.0f);
        statusLabel.repaint();
    }
}

void InstrumentBrowserPanel::clearSearch()
{
    searchBox.clear();
    searchBox.grabKeyboardFocus();
#ifdef ZENITH_USE_SKIA
    searchClearButton->setVisible(false);
#else
    searchClearButton.setVisible(false);
#endif
    updateSearchFilter();
}

zenith::Track* InstrumentBrowserPanel::getSelectedTrack() const
{
    // Get first instrument track from project state
    // TODO(zenith-core#1): Implement proper track selection (selected track in arranger)
    // For now, we'll use the first instrument track

    int numTracks = engine_.getNumTracks();
    for (int i = 0; i < numTracks; ++i)
    {
        auto trackTree = projectState_.getTrackByIndex(i);
        if (trackTree.isValid())
        {
            // Check if it's an instrument track
            juce::String type = trackTree.getProperty(ProjectState::PROP_TYPE).toString();
            if (type == "instrument")
            {
                // Note: This returns nullptr since we can't directly access Track objects from ValueTree
                // The caller should use the trackTree directly instead
                return nullptr; // TODO(zenith-core#1): Refactor to return ValueTree or track ID
            }
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

    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));

    // Background
    if (rowIsSelected)
    {
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::accentPrimary).withAlpha(0.2f));
        g.fillRoundedRectangle(bounds.reduced(2.0f, 1.0f), ZenithLookAndFeel::Metrics::radiusS);
    }
    else if (rowNumber % 2 == 0)
    {
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::backgroundPanel));
        g.fillAll();
    }

    // Get instrument metadata
    auto instrumentId = owner_.instrumentIds_[rowNumber];
    auto* metadata = owner_.instrumentRegistry_.getMetadata(instrumentId);

    if (metadata != nullptr)
    {
        // Draw instrument name
        g.setColour(rowIsSelected ? juce::Colour(ZenithLookAndFeel::Colors::accentPrimary) 
                                  : juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
        g.setFont(ZenithLookAndFeel::getFontBody().withStyle(rowIsSelected ? juce::Font::bold : juce::Font::plain));
        g.drawText(metadata->name, 12, 0, width - 80, height,
                   juce::Justification::centredLeft, true);

        // Draw category badge
        if (metadata->category.isNotEmpty())
        {
            g.setColour(juce::Colour(ZenithLookAndFeel::Colors::backgroundLight));
            juce::Rectangle<float> badge(static_cast<float>(width - 68), 6.0f, 60.0f, static_cast<float>(height - 12));
            g.fillRoundedRectangle(badge, ZenithLookAndFeel::Metrics::radiusS);

            g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
            g.setFont(ZenithLookAndFeel::getFontTiny());
            g.drawText(metadata->category, badge.toNearestInt(), juce::Justification::centred, true);
        }
    }
    else
    {
        // Fallback: just draw ID
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
        g.setFont(ZenithLookAndFeel::getFontBody());
        g.drawText(instrumentId, 12, 0, width - 20, height,
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
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));

    // Background
    if (rowIsSelected)
    {
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::accentPrimary).withAlpha(0.2f));
        g.fillRoundedRectangle(bounds.reduced(2.0f, 1.0f), ZenithLookAndFeel::Metrics::radiusS);
    }
    else if (rowNumber % 2 == 0)
    {
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::backgroundPanel));
        g.fillAll();
    }

    // Draw preset name
    g.setColour(rowIsSelected ? juce::Colour(ZenithLookAndFeel::Colors::accentPrimary) 
                              : juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
    g.setFont(ZenithLookAndFeel::getFontBody().withStyle(rowIsSelected ? juce::Font::bold : juce::Font::plain));
    g.drawText(preset.name, 12, 0, width - 80, height,
               juce::Justification::centredLeft, true);

    // Draw category badge with rounded corners
    if (!preset.category.empty())
    {
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::accentSecondary).withAlpha(0.2f));
        juce::Rectangle<float> badge(static_cast<float>(width - 73), 5.0f, 65.0f, static_cast<float>(height - 10));
        g.fillRoundedRectangle(badge, ZenithLookAndFeel::Metrics::radiusS);

        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::accentSecondary));
        g.setFont(ZenithLookAndFeel::getFontTiny().withStyle(juce::Font::bold));
        g.drawText(preset.category, badge.toNearestInt(), juce::Justification::centred, true);
    }

    // Draw tags as small dots (if present)
    if (!preset.tags.empty() && !rowIsSelected)
    {
        int dotX = 4;
        int dotY = height - 6;
        int dotSize = 3;

        for (size_t i = 0; i < std::min(preset.tags.size(), size_t(3)); ++i)
        {
            g.setColour(juce::Colour(ZenithLookAndFeel::Colors::accentPrimary).withAlpha(0.5f));
            g.fillEllipse(static_cast<float>(dotX), static_cast<float>(dotY),
                         static_cast<float>(dotSize), static_cast<float>(dotSize));
            dotX += dotSize + 2;
        }
    }
}

void InstrumentBrowserPanel::PresetListBoxModel::listBoxItemDoubleClicked(
    int row, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    owner_.onPresetDoubleClicked(row);
}

} // namespace zenith

